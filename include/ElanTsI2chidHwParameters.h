/** @file

  Header of HW Parameters of Elan I2C-HID Touchscreen.
  The touch controller aforementioned includes Elan Gen5 / Gen6 / Gen7 touch series.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsI2chidHwParameters.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_I2CHID_HW_PARAMETERS_H_
#define _ELAN_TS_I2CHID_HW_PARAMETERS_H_

/***************************************************
 * Definitions
 ***************************************************/

// Hello Packet (Normal IAP)
#ifndef ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET
#define ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET	0x20
#endif //ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET

// Hello Packet (Recovery IAP)
#ifndef ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET
#define ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET	0x56
#endif //ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET

//
// Solution ID (High Byte of FW Version)
//

// 3300
const int SOLUTION_ID_EKTH3300x1	= 0x90;
const int SOLUTION_ID_EKTH3300x2	= 0x00;
const int SOLUTION_ID_EKTH3300x3	= 0x01;
const int SOLUTION_ID_EKTH3300x3HV	= 0x02;

// 3900
const int SOLUTION_ID_EKTH3900x1	= 0x10;
const int SOLUTION_ID_EKTH3900x2	= 0x11;
const int SOLUTION_ID_EKTH3900x3	= 0x12;
const int SOLUTION_ID_EKTH3900x3HV	= 0x13;

// 3915
const int SOLUTION_ID_EKTH3915M		= 0x14;

// 3920
const int SOLUTION_ID_EKTH3920		= 0x20;

// 3926
const int SOLUTION_ID_EKTH3260x1	= 0x30;

// 5200
const int SOLUTION_ID_EKTA5200x1	= 0x50;
const int SOLUTION_ID_EKTA5200x2	= 0x51;
const int SOLUTION_ID_EKTA5200x3	= 0x52;

// 53xx for Some Customers
const int SOLUTION_ID_EKTA53XXx1	= 0x55;

// 5312 (A: Active Pen)
const int SOLUTION_ID_EKTA5312x1	= 0x56;
const int SOLUTION_ID_EKTA5312x2	= 0x57;
const int SOLUTION_ID_EKTA5312x3	= 0x58;

// 6315
const int SOLUTION_ID_EKTH6315x1	= 0x61;
const int SOLUTION_ID_EKTH6315x2	= 0x62;

// 6315 remark to 5015M
const int SOLUTION_ID_EKTH6315to5015M = 0x59;

// 6315 remark to 3915
const int SOLUTION_ID_EKTH6315to3915P = 0x15;

// 6308
const int SOLUTION_ID_EKTH6308x1	= 0x63;

// 7315
const int SOLUTION_ID_EKTH7315x1	= 0x64;
const int SOLUTION_ID_EKTH7315x2	= 0x65;

// 7318
const int SOLUTION_ID_EKTH7318x1	= 0x67;

//
// High Byte of Boot Code Version (in Recovery Mode)
//

// 5312b
const int BC_VER_H_BYTE_H_NIBBLE_FOR_EKTA5312b_I2C_USB	= 0x6;
const int BC_VER_H_BYTE_FOR_EKTA5312bx1_I2CHID	= 0xA5;
const int BC_VER_H_BYTE_FOR_EKTA5312bx2_I2CHID	= 0xB5;
const int BC_VER_H_BYTE_FOR_EKTA5312bx3_I2CHID	= 0xC5;

// 5312c
const int BC_VER_H_BYTE_H_NIBBLE_FOR_EKTA5312c_I2C_USB	= 0x7;
const int BC_VER_H_BYTE_FOR_EKTA5312cx1_I2CHID	= 0xA6;
const int BC_VER_H_BYTE_FOR_EKTA5312cx2_I2CHID	= 0xB6;
const int BC_VER_H_BYTE_FOR_EKTA5312cx3_I2CHID	= 0xC6;

// 6315
const int BC_VER_H_BYTE_H_NIBBLE_FOR_EKTA6315_I2C_USB	= 0x8;
const int BC_VER_H_BYTE_FOR_EKTA6315_I2CHID	= 0xA7;

// 6315 to 5015M
const int BC_VER_H_BYTE_FOR_EKTH6315_TO_5015M_I2CHID = 0xE6;

// 6315 to 3915P
const int BC_VER_H_BYTE_FOR_EKTH6315_TO_3915P_I2CHID = 0xF6;

// 6308
const int BC_VER_H_BYTE_FOR_EKTA6308_I2CHID	= 0xA8;

// 7315
const int BC_VER_H_BYTE_FOR_EKTA7315_I2CHID	= 0xA9;

#endif //_ELAN_TS_I2CHID_HW_PARAMETERS_H_
