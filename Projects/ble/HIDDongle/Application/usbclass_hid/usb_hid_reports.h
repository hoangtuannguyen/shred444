/***********************************************************************************

  Filename:		usb_hid_reports.h

  Description:	Definitions and prototypes for HID reports

***********************************************************************************/

#ifndef USBHIDREPORTS_H
#define USBHIDREPORTS_H

#include "hal_types.h"

typedef struct {
    uint8 ledStatus;
} KEYBOARD_OUT_REPORT;

typedef struct {
    uint8 modifiers;
    uint8 reserved;
    uint8 pKeyCodes[6];
} KEYBOARD_IN_REPORT;

typedef struct {
    uint8 buttons;
    int8 dX;
    int8 dY;
    int8 dZ;
} MOUSE_IN_REPORT;


typedef struct {
    KEYBOARD_OUT_REPORT keyboardOutReport;
    KEYBOARD_IN_REPORT keyboardInReport;
    MOUSE_IN_REPORT mouseInReport;
    uint8 keyboardProtocol;
    uint8 mouseProtocol;
    uint16 keyboardIdleRate;
    uint16 mouseIdleRate;
} HID_DATA;


extern HID_DATA hidData;


uint8 hidSendKeyboardInReport(void);
uint8 hidSendMouseInReport(void);
void hidUpdateKeyboardInReport(KEYBOARD_IN_REPORT *pNewReport);
void hidUpdateMouseInReport(MOUSE_IN_REPORT *pNewReport);
void hidShowKeyboardLeds(void);

/*
+------------------------------------------------------------------------------
|  Copyright 2004-2010 Texas Instruments Incorporated. All rights reserved.
|
|  IMPORTANT: Your use of this Software is limited to those specific rights
|  granted under the terms of a software license agreement between the user who
|  downloaded the software, his/her employer (which must be your employer) and
|  Texas Instruments Incorporated (the "License"). You may not use this Software
|  unless you agree to abide by the terms of the License. The License limits
|  your use, and you acknowledge, that the Software may not be modified, copied
|  or distributed unless embedded on a Texas Instruments microcontroller or used
|  solely and exclusively in conjunction with a Texas Instruments radio
|  frequency transceiver, which is integrated into your product. Other than for
|  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
|  works of, modify, distribute, perform, display or sell this Software and/or
|  its documentation for any purpose.
|
|  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
|  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
|  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
|  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
|  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
|  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
|  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING
|  BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
|  CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
|  SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
|  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
|
|  Should you have any questions regarding your right to use this Software,
|  contact Texas Instruments Incorporated at www.TI.com.
|
+------------------------------------------------------------------------------
*/

#endif
