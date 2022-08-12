/** @file

  Header of Function Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsI2chidUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_I2CHID_UTILITY_H_
#define _ELAN_TS_I2CHID_UTILITY_H_

#include <stdio.h>
#include <stdlib.h>
#include "ElanTsI2chidHwParameters.h" // HW Parameters for Elan Gen5 / Gen6 / Gen7 Touch Controllers

/***************************************************
 * Definitions
 ***************************************************/

// Slave Address
#ifndef ELAN_I2C_SLAVE_ADDR
#define ELAN_I2C_SLAVE_ADDR	0x20
#endif //ELAN_I2C_SLAVE_ADDR

// Calibration Response Timeout
#ifndef ELAN_READ_CALI_RESP_TIMEOUT_MSEC
#define	ELAN_READ_CALI_RESP_TIMEOUT_MSEC	10000 //30000
#endif //ELAN_READ_CALI_RESP_TIMEOUT_MSEC

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

// HID Raw I/O
extern int __hidraw_write(unsigned char* buf, int len, int timeout_ms);
extern int __hidraw_read(unsigned char* buf, int len, int timeout_ms);

/*******************************************
 * Function Prototype
 ******************************************/

// Power Status
int send_set_power_status_command(int mode);

// FW ID
int send_fw_id_command(void);
int read_fw_id_data(void);
int get_fw_id_data(unsigned short *p_fw_id);

// FW Version
int send_fw_version_command(void);
int read_fw_version_data(bool quiet /* Silent Mode */);
int get_fw_version_data(unsigned short *p_fw_version);

// Solution ID
int get_solution_id(unsigned char *p_solution_id);

// Test Version
int send_test_version_command(void);
int read_test_version_data(void);
int get_test_version_data(unsigned short *p_test_version);

// Boot Code Version
int send_boot_code_version_command(void);
int read_boot_code_version_data(void);
int get_boot_code_version_data(unsigned short *p_bc_version);

// Calibration
int send_rek_command(void);
int receive_rek_response(void);

// Test Mode
int send_enter_test_mode_command(void);
int send_exit_test_mode_command(void);

// ROM Data
int send_read_rom_data_command(unsigned short addr, bool recovery, unsigned char info);
int receive_rom_data(unsigned short *p_rom_data);

// Bulk ROM Data
int send_show_bulk_rom_data_command(unsigned short addr, unsigned short len);

// Bulk ROM Data (in Boot Code)
int send_show_bulk_rom_data_command(unsigned short addr);

int receive_bulk_rom_data(unsigned short *p_rom_data);

// IAP Mode
int send_write_flash_key_command(void);
int send_enter_iap_command(void);
int send_slave_address(void);

// Frame Data
int write_frame_data(int data_offset, int data_len, unsigned char *frame_buf, int frame_buf_size);

// Flash Write
int send_flash_write_command(void);
int receive_flash_write_response(void);

// Hello Packet
int send_request_hello_packet_command(void);

#endif //_ELAN_TS_I2CHID_UTILITY_H_
