/** @file

  Header of Firmware File I/O Utility for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFwFileIoUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_GEN8_TS_FW_FILE_IO_UTILITY_H_
#define _ELAN_GEN8_TS_FW_FILE_IO_UTILITY_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/***************************************************
 * Definitions
 ***************************************************/

// eKTL FW Page Size
#ifndef ELAN_EKTL_FW_PAGE_SIZE
#define ELAN_EKTL_FW_PAGE_SIZE			2056	// 4 (address) + 2048 (data) + 4 (checksum) 
#endif //ELAN_EKTL_FW_PAGE_SIZE

// Frame Page Data Size
#ifndef ELAN_EKTL_FW_PAGE_DATA_SIZE
#define ELAN_EKTL_FW_PAGE_DATA_SIZE		2048	// 0x800 (in byte)
#endif //ELAN_EKTL_FW_PAGE_DATA_SIZE

// Erase Section Count Max
#ifndef MAX_ERASE_SECTION_COUNT
#define MAX_ERASE_SECTION_COUNT		256			// (2056-6-4-16-4-3)/(4+4)=252.875
#endif //MAX_ERASE_SECTION_COUNT

/***************************************************
 * Macro Function Definitions
 ***************************************************/

#define FOUR_BYTE_ARRAY_TO_UINT(four_byte_array)	\
	(unsigned int)(((four_byte_array)[0]) | ((four_byte_array)[1] << 8) | ((four_byte_array)[2] << 16) | ((four_byte_array)[3] << 24))

/***************************************************
 * Declaration of Data Structure
 ***************************************************/

// Erase Section
struct erase_section
{
    unsigned int address;		// 32-bit, or 4-byte
    unsigned int page_count;	// 32-bit, or 4-byte
};
typedef struct erase_section ERASE_SECTION, *P_ERASE_SECTION;

// Erase Script
struct erase_script
{
    unsigned int nArrayVerLibHex2Ektl[4];						// 4 * 4-byte
    unsigned int nEraseSectionCount;							// 4-byte
    struct erase_section EraseSection[MAX_ERASE_SECTION_COUNT];	// 256 * 8-byte
};
typedef struct erase_script ERASE_SCRIPT, *P_ERASE_SCRIPT;

/***************************************************
 * Global Data Structure Declaration
 ***************************************************/

/***************************************************
 * Global Variables Declaration
 ***************************************************/

// Firmware File Information
extern int g_firmware_fd;

/***************************************************
 * Extern Variables Declaration
 ***************************************************/

/***************************************************
 * Function Prototype
 ***************************************************/

/*
 * The functions in this header are based on the following functions in ElanTsFwFileIoUtility.h.
 * Before using functions in here, be sure to include ElanTsFwFileIoUtility.h and invoke its functions.
 *
 * int open_firmware_file(char *filename, size_t filename_len);
 * int close_firmware_file(void);
 * int get_firmware_size(int *firmware_size);
 * int retrieve_data_from_firmware(unsigned char *data, int data_size);
 */

// Validate eKTL FW
int validate_ektl_fw(bool *p_result);

// eKTL FW File I/O
int compute_ektl_fw_page_number(int firmware_size);
int compute_ektl_page_number(int firmware_size);

// eKTL Erase Script
int get_ektl_erase_script(struct erase_script *p_erase_script, size_t erase_script_size);

// eKTL Page Data
int get_page_data_from_ektl_firmware(unsigned char page_index, unsigned char *p_ektl_page_buf, size_t ektl_page_buf_size);

// Remark ID
int get_remark_id_from_ektl_firmware(unsigned char *p_gen8_remark_id_buf, size_t gen8_remark_id_buf_size);

#endif //_ELAN_GEN8_TS_FW_FILE_IO_UTILITY_H_
