/** @file

  Header of Firmware File I/O Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsFwFileIoUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_FW_FILE_IO_UTILITY_H_
#define _ELAN_TS_FW_FILE_IO_UTILITY_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/***************************************************
 * Definitions
 ***************************************************/

// File Length
#ifndef FILE_NAME_LENGTH_MAX
#define FILE_NAME_LENGTH_MAX	256
#endif //FILE_NAME_LENGTH_MAX

// Firmware Page Size
#ifndef ELAN_FIRMWARE_PAGE_SIZE
#define ELAN_FIRMWARE_PAGE_SIZE 132 /* (1+64+1)*2=132 byte */
#endif //ELAN_FIRMWARE_PAGE_SIZE

// Frame Page Data Size
#ifndef ELAN_FIRMWARE_PAGE_DATA_SIZE
#define ELAN_FIRMWARE_PAGE_DATA_SIZE	128  // 0x40 (in word)
#endif //ELAN_FIRMWARE_PAGE_DATA_SIZE

/***************************************************
 * Macro Function Definitions
 ***************************************************/

#define TWO_BYTE_ARRAY_TO_WORD(two_byte_array)	\
	(unsigned short)(((two_byte_array)[0]) | ((two_byte_array)[1] << 8))

#define REVERSE_TWO_BYTE_ARRAY_TO_WORD(two_byte_array)	\
	(unsigned short)(((two_byte_array)[0] << 8) | ((two_byte_array)[1]))

/*******************************************
 * Global Data Structure Declaration
 ******************************************/

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

/*******************************************
 * Function Prototype
 ******************************************/

// Firmware File I/O
int open_firmware_file(char *filename, size_t filename_len);
int close_firmware_file(void);
int get_firmware_size(int *firmware_size);
int compute_firmware_page_number(int firmware_size);
int retrieve_data_from_firmware(unsigned char *data, int data_size);

// Remark ID
int get_remark_id_from_firmware(unsigned short *p_remark_id);

#endif //_ELAN_TS_FW_FILE_IO_UTILITY_H_
