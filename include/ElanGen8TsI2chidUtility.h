/** @file

  Header of Function Utility for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsI2chidUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_GEN8_TS_I2CHID_UTILITY_H_
#define _ELAN_GEN8_TS_I2CHID_UTILITY_H_

#include <stdio.h>
#include <stdlib.h>

/***************************************************
 * Definitions
 ***************************************************/

/***************************************************
 * Global Data Structure Declaration
 ***************************************************/

/***************************************************
 * Global Variables Declaration
 ***************************************************/

// Debug
extern bool g_debug;

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(fmt, argv...) if(g_debug) printf(fmt, ##argv)
#endif //DEBUG_PRINTF

#ifndef ERROR_PRINTF
#define ERROR_PRINTF(fmt, argv...) fprintf(stderr, fmt, ##argv)
#endif //ERROR_PRINTF

/***************************************************
 * Extern Variables Declaration
 ***************************************************/

// Write Command
extern int write_cmd(unsigned char *cmd_buf, int len, int timeout_ms);

// Read Data
extern int read_data(unsigned char *data_buf, int len, int timeout_ms);

// Write Vendor Command
extern int write_vendor_cmd(unsigned char *cmd_buf, int len, int timeout_ms);

// HID Raw I/O
extern int __hidraw_write(unsigned char* buf, int len, int timeout_ms);
extern int __hidraw_read(unsigned char* buf, int len, int timeout_ms);

/***************************************************
 * Function Prototype
 ***************************************************/

// Test Version
int gen8_read_test_version_data(void);
int gen8_get_test_version_data(unsigned short *p_test_version);

// ROM Data
int gen8_send_read_rom_data_command(unsigned int addr, unsigned char data_len);
int gen8_receive_rom_data(unsigned int *p_rom_data);

// IAP Mode
int send_gen8_write_flash_key_command(void);

// Erase Flash Section
int send_erase_flash_section_command(unsigned int address, unsigned short page_count);
int receive_erase_flash_section_response(void);

#endif //_ELAN_GEN8_TS_I2CHID_UTILITY_H_
