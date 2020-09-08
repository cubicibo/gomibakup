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
#define PAL_DEBUG PAL_LINE(GPIOG, 4u)

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
/*
// 0 : clock M2 only during reading, perform all writes repeatidly between all reads
// 1 : Continuously clock M2 during USB transfers
*/
static uint8_t dumpingMode = 0;
static bool error = false;

static bool dataBusRead = true; //pins are input by default
static bool dump = false;

#define MAX_BANK_SIZE 0x8000
static volatile uint8_t data[MAX_BANK_SIZE]; //Buffer
static uint32_t data_offset;

#define NB_COMMAND_MAX_SCRIPT 5000
static command unrolledScript[NB_COMMAND_MAX_SCRIPT]; //at most 5000 instructions
static uint16_t unrolledSize;

static uint16_t currentCommand = 0; //command pointer

//TIM1N on PB15 AF1
#define PWMDRV PWMD1

static PWMConfig pwmCfgTim1N = { 90000000, 50, NULL, //1.8MHz PWM
	  {
		 {PWM_COMPLEMENTARY_OUTPUT_DISABLED, NULL},
		 {PWM_COMPLEMENTARY_OUTPUT_DISABLED, NULL},
		 {PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH, NULL},
		 {PWM_COMPLEMENTARY_OUTPUT_DISABLED, NULL}
	  }, 0,0
	#if STM32_PWM_USE_ADVANCED
	  ,0 // TIM BDTR (break & dead-time) register initialization data.
	#endif
};

//User from Micropython has requested to initiate the dumping sequence
void request_dump() {
	error = false;
	dump = true;
	data_offset = 0;
	palClearLine(PAL_DEBUG);
}

void reset_script() {
	hw_io_prg(true);
	error = false;
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
	uint32_t bytesSent = 0;
	//lock to thread
	uint32_t prgSize = 0;
	for (cnt = 0; cnt < unrolledSize; cnt++) {
		if (unrolledScript[cnt].read)
			prgSize += unrolledScript[cnt].val;
	}
	chprintf((BaseSequentialStream *)&USB_SERIAL, "%d\n", prgSize);
	chThdSleepMilliseconds(500);
	chSysLock();
	for (cnt = 0; cnt < unrolledSize; cnt++) {
		if (unrolledScript[cnt].read) {

			read_prg_f(unrolledScript[cnt].addr, unrolledScript[cnt].val);

			/*palSetLineMode(PAL_M2, PAL_MODE_ALTERNATE(1)); //HW PWM
			bytesSent += chnWrite((BaseSequentialStream *)&USB_SERIAL, data+0x4000*cnt, sizeof(uint8_t)*unrolledScript[cnt].val);//, TIME_MS2I(750));
			palSetLineMode(PAL_M2, PAL_STM32_MODE_OUTPUT);
			data_offset = 0;*/
			if (cnt+1 < unrolledSize && unrolledScript[cnt+1].val+data_offset > MAX_BANK_SIZE) {
				chnWrite((BaseSequentialStream *)&USB_SERIAL, data, sizeof(uint8_t)*data_offset);
				data_offset = 0;
			}
		}
		else {
			write_prg(unrolledScript[cnt].addr, (uint8_t)unrolledScript[cnt].val);
		}
	}
	if (data_offset > 0)
		chnWrite((BaseSequentialStream *)&USB_SERIAL, data, sizeof(uint8_t)*data_offset);
	chSysUnlock();
}

void execute_script_intermittent() {

	bool *todo;
	todo = (bool*)chHeapAlloc(NULL,sizeof(bool)*unrolledSize);

	uint16_t cnt;
	//lock to thread
	chSysLock();
	for (cnt = 0; cnt < unrolledSize; cnt++) {
		if (unrolledScript[cnt].read && todo[cnt]) {
			//send data if buffer is full
			if (unrolledScript[cnt].val+data_offset > MAX_BANK_SIZE) {
				if(chnWrite((BaseSequentialStream *)&USB_SERIAL, data, sizeof(uint8_t)*data_offset) != data_offset) {
					error = true; //chnWrite missed bytes -> file on host is incomplete
				}
				data_offset = 0;
				cnt = 0; // restart over all writes
			}
			else {
				read_prg_f(unrolledScript[cnt].addr, unrolledScript[cnt].val);
				todo[cnt] = false;
			}
		}
		else if (!unrolledScript[cnt].read) {
			write_prg(unrolledScript[cnt].addr, (uint8_t)unrolledScript[cnt].val);
		}
	}
	chSysUnlock();
	chHeapFree(todo);
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
		chThdSleepMilliseconds(2500);

		//if user requested a dump from Micropython and the script size is proper
		if (dump && unrolledSize > 0) {
				dump = false;
				currentCommand = 0;
				execute_script();
			}
		else
			dump = false;
	}
}

void start_thread() {
	//pwmStart(&PWMDRV, &pwmCfgTim1N);
	//pwmEnableChannel(&PWMDRV, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMDRV, 5000));

	//pwmEnableChannel(&PWMDRV, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMDRV, 5000));
	//pwmStop(&PWMDRV);
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
	uint8_t tmp = 0;
	uint16_t k;
	uint16_t i;
	uint32_t j;

	chSysUnconditionalLock();

	for (i=0, j = 0; i < size; j++) {
		palWriteBus(&cpuABus, addr+i);
		palSetLine(PAL_CPU_RW);
		palSetLine(PAL_M2);
		palWriteLine(PAL_nROMSEL, (~(addr>>15u))&0x01);

		for (k=0; k<7;k++) //6
			__asm__("  nop;");

		if(j&0x1) {
			if (tmp == palReadBus(&cpuDBus))
				data[data_offset+i++] = tmp;
			else
				j-=2;
		}
		else
			tmp = palReadBus(&cpuDBus);

		palClearLine(PAL_M2);
		palSetLine(PAL_nROMSEL);
		//palClearLine(PAL_CPU_RW);

		for (k=0; k<3;k++) //4
			__asm__("  nop;");
	}
	data_offset += size;
	chSysUnconditionalUnlock();
}

void write_prg(uint16_t addr, uint8_t val) {
	chSysUnconditionalLock();

	uint8_t k;

	palWriteBus(&cpuABus, addr);
	palClearLine(PAL_CPU_RW);
	hw_io_prg(false);
	palWriteBus(&cpuDBus, val);
	palSetLine(PAL_M2);
	palWriteLine(PAL_nROMSEL, (~(addr>>15u))&0x01);

	for (k=0; k<6;k++)
		__asm__("  nop;");

	palClearLine(PAL_M2);
	palSetLine(PAL_nROMSEL);

	for (k=0; k<4;k++)
		__asm__("  nop;");

	chSysUnconditionalUnlock();

}
