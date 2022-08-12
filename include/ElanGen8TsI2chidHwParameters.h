/** @file

  Header of HW Parameters of Elan Gen8 I2C-HID Touchscreen.
  The touch controller aforementioned includes Gen8 touch series.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsI2chidHwParameters.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_GEN8_TS_I2CHID_HW_PARAMETERS_H_
#define _ELAN_GEN8_TS_I2CHID_HW_PARAMETERS_H_

/***************************************************
 * Definitions
 ***************************************************/

// Hello Packet (Normal IAP)
#ifndef ELAN_GEN8_I2CHID_NORMAL_MODE_HELLO_PACKET
#define ELAN_GEN8_I2CHID_NORMAL_MODE_HELLO_PACKET	0x21
#endif //ELAN_GEN8_I2CHID_NORMAL_MODE_HELLO_PACKET

// Hello Packet (Recovery IAP)
#ifndef ELAN_GEN8_I2CHID_RECOVERY_MODE_HELLO_PACKET
#define ELAN_GEN8_I2CHID_RECOVERY_MODE_HELLO_PACKET	0x57
#endif //ELAN_GEN8_I2CHID_RECOVERY_MODE_HELLO_PACKET

//
// High Byte of Boot Code Version (in Recovery Mode)
//

// EM32F901
const int BC_VER_H_BYTE_FOR_EM32F901_I2CHID	= 0x95;

// EM32F902
const int BC_VER_H_BYTE_FOR_EM32F902_I2CHID	= 0x9C;

#endif //_ELAN_GEN8_TS_I2CHID_HW_PARAMETERS_H_
