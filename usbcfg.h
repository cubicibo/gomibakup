/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef USBCFG_H
#define USBCFG_H

#define USE_TWO_USB_SERIAL

//power asked to the computer in 2mA incerement (min 100mA, max 500mA)
#define USB_POWER           150          //(300mA)

#define USB_DEVICE_VID      0x1D50
#define USB_DEVICE_PID      0x6018
#define USB_VENDOR_NAME     "BGW"
#define USB_DEVICE_NAME     "gomibakup"
#define USB_SERIAL_NUMBER   "gomibakup_"
#define USB_SERIAL1_NAME    "gomibakupPython"
#ifdef USE_TWO_USB_SERIAL
#define USB_SERIAL2_NAME    "gomibakupData"
#endif /* USE_TWO_USB_SERIAL */

#define USB_GDB             SDU1
#ifdef USE_TWO_USB_SERIAL
#define USB_SERIAL          SDU2
#endif /* USE_TWO_USB_SERIAL */

typedef enum{
    GDB_INTERFACE = 0,
    SERIAL_INTERFACE,
    NUM_INTERFACES,
}interface_name_t;

typedef enum{
    CONTROL_LINE_RTS = 0,
    CONTROL_LINE_DTR,
    NUM_CONTROL_LINE,
}control_line_t;

#define USB_DATA_SIZE                   0x40

extern const USBConfig usbcfg;
extern SerialUSBDriver SDU1;
extern const SerialUSBConfig serusbcfg1;

#ifdef USE_TWO_USB_SERIAL
extern const SerialUSBConfig serusbcfg2;
extern SerialUSBDriver SDU2;
#endif /* USE_TWO_USB_SERIAL */

/*
* Initializes two serial-over-USB CDC drivers and starts and connects the USB.
*/
void usbSerialStart(void);

/**
 * @brief Returns if the USB is configured or not
 * @return 1 or 0
 */
uint8_t isUSBConfigured(void);

/**
 * @brief   Returns the state of the selected controle line of the selected interface
 * 
 * @param interface     See interface_name_t
 * @param rts_dtr       See control_line_object_t
 * 
 * @return              The state desired
 */
uint8_t getControlLineState(interface_name_t interface, control_line_t rts_dtr);

#endif  /* USBCFG_H */

/** @} */
