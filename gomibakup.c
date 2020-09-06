/*
 * gomibakup.c
 */

#include "ch.h"
#include "hal.h"
#include "gomibakup.h"
#include "usbcfg.h"
#include "chprintf.h"

/*
 * PB15 M2
 * PA15 CPU R/W
 * PC10 /ROMSEL (/A15 + /M2)
 */
////CTRL PINS
#define PAL_M2 PAL_LINE(GPIOB,15u)
#define PAL_CPU_RW PAL_LINE(GPIOA,15u)
#define PAL_nROMSEL PAL_LINE(GPIOC,10u)

///// BUSES
static IOBus cpuDBus = {GPIOA, 0x00FF, 0};
static IOBus cpuABus =  {GPIOB, 0x7FFF, 0}; //PB15 is M2

static IOBus ppuDBus = {GPIOD, 0x00FF, 0};
static IOBus ppuABus = {GPIOE, 0xFFFF, 0};

///typedef
typedef struct script { // R/W 0xADDR 0xVAL
	bool read;
	uint16_t addr;
	uint16_t val;
} command;

////Vars
static bool dataBusRead = true; //pins are input by default
static bool dump = false;

#define MAX_BANK_SIZE 0x8000
static uint8_t data[MAX_BANK_SIZE]; //Buffer
static uint16_t data_offset;

#define NB_COMMAND_MAX_SCRIPT 5000
static command unrolledScript[NB_COMMAND_MAX_SCRIPT]; //at most 5000 instructions
static uint16_t unrolledSize;

static uint16_t currentCommand = 0; //command pointer

//User from Micropython has requested to initiate the dumping sequence
void request_dump() {
	dump = true;
	currentCommand = 0;
	data_offset = 0;
}

void reset_script() {
	hw_io_prg(true);
	dump = false;
	unrolledSize = 0;
	currentCommand = 0;
	data_offset = 0;
}

void store_command(bool read, uint16_t a, uint16_t val) {
	unrolledScript[unrolledSize].read = read;
	unrolledScript[unrolledSize].addr = a;
	unrolledScript[unrolledSize].val = val;
	unrolledSize++;
}

void execute_script() {

	uint16_t cnt;
	//lock to thread
	chSysLock();

	for (cnt = 0; cnt < unrolledSize; cnt++) {
		if (unrolledScript[cnt].read) {
			read_prg_f(unrolledScript[cnt].addr, unrolledScript[cnt].val);
			//empty buffer here?
			//chnWrite((BaseSequentialStream *)&USB_SERIAL, data, sizeof(uint8_t)*data_offset);
		}
		else {
			write_prg(unrolledScript[cnt].addr, (uint8_t)unrolledScript[cnt].val);
		}
	}
	chSysUnlock();
}

static THD_WORKING_AREA(waDumpCtrlTh, 128); // @suppress("Symbol is not resolved")
static THD_FUNCTION(DumpCtrl, arg) {

	unrolledScript[0].read = 1;
	unrolledScript[0].addr = 0x8000;
	unrolledScript[0].val = 0x4000;
	unrolledScript[1].read = 1;
	unrolledScript[1].addr = 0xC000;
	unrolledScript[1].val = 0x4000;
	unrolledSize = 2;

	while(true) {
		if (dump && currentCommand < unrolledSize) {
				dump = false;
				currentCommand = 0;
				execute_script();
			}
		else
			dump = false;
		chThdSleepMilliseconds(1000);
	}
}

void start_thread() {
	chThdCreateStatic(waDumpCtrlTh, sizeof(waDumpCtrlTh), NORMALPRIO, DumpCtrl, NULL);
}

void fill_command(uint16_t size, bool *action, uint16_t *addr, uint16_t *val) {
	for (uint16_t k = 0; k < size && k < NB_COMMAND_MAX_SCRIPT; k++) {
		action[k] = unrolledScript[k].read;
		addr[k] = unrolledScript[k].addr;
		val[k] = unrolledScript[k].val;
	}
}

//switch pins mode for data bus
void hw_io_prg(bool read) {
	if (read != dataBusRead) {
		dataBusRead = read;
		if (read) {
			palSetBusMode(&cpuDBus, PAL_STM32_MODE_INPUT);
		}
		else {
			palSetBusMode(&cpuDBus, PAL_STM32_MODE_OUTPUT);
		}
	}
}

void read_prg_f(uint16_t addr, uint16_t size) {
	hw_io_prg(true);
	data_offset = 0;
	uint16_t k = 0;

	//chprintf((BaseSequentialStream *)&USB_GDB,"HI\n");

	//chSysLock();
	for (uint16_t j = 0; j < size; j++) {
		palWriteBus(&cpuABus, addr+j);
		palSetLine(PAL_CPU_RW);
		palSetLine(PAL_M2);
		palWriteLine(PAL_nROMSEL, !(((addr)>>15u) & 0x01));
		for (k=0; k<8;k++)
			__asm__("  nop;");
		data[data_offset] = palReadBus(&cpuDBus);
		data_offset++;
		palClearLine(PAL_M2);
		palSetLine(PAL_nROMSEL);
		for (k=0; k<4;k++)
			__asm__("  nop;");
	}
	//chSysUnlock();
}

void write_prg(uint16_t addr, uint8_t val) {
	uint8_t k;

	palWriteBus(&cpuABus, addr);
	palClearLine(PAL_CPU_RW);
	hw_io_prg(false);
	palWriteBus(&cpuDBus, val);
	palSetLine(PAL_M2);
	palWriteLine(PAL_nROMSEL, !(((addr)>>15u) & 0x01));

	for (k=0; k<8;k++)
		__asm__("  nop;");

	palClearLine(PAL_M2);
	palSetLine(PAL_nROMSEL);

	for (k=0; k<4;k++)
		__asm__("  nop;");
}
