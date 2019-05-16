/** @file

  Header of Function Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsFuncUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_FUNC_UTILITY_H_
#define _ELAN_TS_FUNC_UTILITY_H_

#include <stdio.h>
#include <stdlib.h>

/***************************************************
 * Definitions
 ***************************************************/

// Slave Address
#ifndef ELAN_I2C_SLAVE_ADDR
#define ELAN_I2C_SLAVE_ADDR	0x20
#endif //ELAN_I2C_SLAVE_ADDR

// Calibration Response Timeout
#ifndef ELAN_READ_CALI_RESP_TIMEOUT_MSEC
#define	ELAN_READ_CALI_RESP_TIMEOUT_MSEC		30000
#endif //ELAN_READ_CALI_RESP_TIMEOUT_MSEC

// Information Page Address
#ifndef ELAN_INFO_PAGE_MEMORY_ADDR
#define	ELAN_INFO_PAGE_MEMORY_ADDR	0x8040
#endif //ELAN_INFO_PAGE_MEMORY_ADDR

// Information Page Address to Write
#ifndef ELAN_INFO_PAGE_WRITE_MEMORY_ADDR
#define	ELAN_INFO_PAGE_WRITE_MEMORY_ADDR	0x0040
#endif //ELAN_INFO_PAGE_WRITE_MEMORY_ADDR

// Firmware Page Size
#ifndef ELAN_FIRMWARE_PAGE_SIZE
#define ELAN_FIRMWARE_PAGE_SIZE 132 /* (1+64+1)*2=132 byte */
#endif //ELAN_FIRMWARE_PAGE_SIZE

// Frame Page Data Size
#ifndef ELAN_FIRMWARE_PAGE_DATA_SIZE
#define ELAN_FIRMWARE_PAGE_DATA_SIZE	128  // 0x40 (in word)
#endif //ELAN_FIRMWARE_PAGE_DATA_SIZE

// ELAN I2C-HID Buffer Size for Data
#ifndef ELAN_I2CHID_DATA_BUFFER_SIZE
#define	ELAN_I2CHID_DATA_BUFFER_SIZE	0x3F /* 67 -2(In Report Size) - 1 (Report ID) - 1(Data_Length)  = 63 Byte */
#endif //ELAN_I2CHID_DATA_BUFFER_SIZE

// ELAN I2C-HID Frame Size for Page Read
#ifndef ELAN_I2CHID_READ_PAGE_FRAME_SIZE
#define	ELAN_I2CHID_READ_PAGE_FRAME_SIZE	0x3C /* 63 - 1(Packet Header 0x99) - 1(Packet Index) -1(Data Length) = 60 Byte */
#endif //ELAN_I2CHID_READ_PAGE_FRAME_SIZE

// ELAN I2C-HID Buffer Size for IAP
#ifndef ELAN_I2CHID_PAGE_FRAME_SIZE
#define ELAN_I2CHID_PAGE_FRAME_SIZE				0x1C /* 33-3(3-Byte Vendor Command)-1(ReportID)=29 Byte=>28 Byte(14Word)*/
#endif //ELAN_I2CHID_PAGE_FRAME_SIZE

// Hello Packet (Normal IAP)
#ifndef ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET
#define ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET	0x20
#endif //ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET

// Hello Packet (Recovery IAP)
#ifndef ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET
#define ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET	0x56
#endif //ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET

/*******************************************
 * Global Data Structure Declaration
 ******************************************/

/*
 * Power MODE DEFINITION
 */
enum POWER_MODE
{
    SLEEP = 0,
    IDLE = 1,
    NORMAL_SCAN = 8
};

/*******************************************
 * Global Variables Declaration
 ******************************************/

// Debug
extern bool g_debug;

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(fmt, argv...) if(g_debug) printf(fmt, ##argv)
#endif //DEBUG_PRINTF

#ifndef ERROR_PRINTF
#define ERROR_PRINTF(fmt, argv...) fprintf(stderr, fmt, ##argv)
#endif //ERROR_PRINTF

/*******************************************
 * Extern Variables Declaration
 ******************************************/

// Write Command
extern int write_cmd(unsigned char *cmd_buf, int len, int timeout_ms);

// Read Data
extern int read_data(unsigned char *data_buf, int len, int timeout_ms);

// Write Vendor Command
extern int write_vendor_cmd(unsigned char *cmd_buf, int len, int timeout_ms);

/*******************************************
 * Function Prototype
 ******************************************/

// Power Status
int send_set_power_status_command(int mode);

// FW ID
int send_fw_id_command(void);
int read_fw_id_data(void);

// FW Version
int send_fw_version_command(void);
int read_fw_version_data(void);

// Test Version
int send_test_version_command(void);
int read_test_version_data(void);

// Boot Code Version
int send_boot_code_version_command(void);
int read_boot_code_version_data(void);

// Calibration
int send_rek_command(void);
int receive_rek_response(void);

// Test Mode
int send_enter_test_mode_command(void);
int send_exit_test_mode_command(void);

// Bulk ROM Data
int send_show_bulk_rom_data_command(unsigned short addr, unsigned short len);

// IAP Mode
int send_enter_iap_command(void);
int send_slave_address(void);

// Flash Write
int send_flash_write_command(void);
int receive_flash_write_response(void);

// Hello Packet
int send_request_hello_packet_command(void);

#endif //_ELAN_TS_FUNC_UTILITY_H_
