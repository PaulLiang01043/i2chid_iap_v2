/** @file

  Header of APIs of Firmware Update Flow for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFwUpdateFlow.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_GEN8_TS_FW_UPDATE_FLOW_H_
#define _ELAN_GEN8_TS_FW_UPDATE_FLOW_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "ElanTsFwUpdateFlow.h" // message_mode_t

/***************************************************
 * Definitions
 ***************************************************/

#if 0 //ndef __ENABLE_GEN8_REMARK_ID_CHECK__
#define __ENABLE_GEN8_REMARK_ID_CHECK__
#endif //__ENABLE_GEN8_REMARK_ID_CHECK__

/***************************************************
 * Extern Variables Declaration
 ***************************************************/

/***************************************************
 * Function Prototype
 ***************************************************/

// Firmware Information
int gen8_get_firmware_information(message_mode_t msg_mode);

// Calibration Counter
int gen8_get_calibration_counter(message_mode_t msg_mode);

// Remark ID Check
int gen8_check_remark_id(bool recovery);

// Firmware Update
int gen8_update_firmware(char *filename, size_t filename_len, bool recovery, int skip_action_code);

#endif //_ELAN_GEN8_TS_FW_UPDATE_FLOW_H_
