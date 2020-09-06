/*
 * main.c
 *  gomibakup
 */


/* ChibiOS */
#include "ch.h"
#include "hal.h"
//#include "chprintf.h"
#include "usbcfg.h"
#include "mp_platform.h"
#include "gomibakup.h"

/* main thread */
int main(void) {
	//OS init
	halInit();
	chSysInit();

	//Start MPY last because we want to make sure the other threads are ready by the time micropython runs.
	usbSerialStart();
	micropythonStart();
	start_thread();

	while (true) {
		chThdSleepSeconds(10);
	}
}
