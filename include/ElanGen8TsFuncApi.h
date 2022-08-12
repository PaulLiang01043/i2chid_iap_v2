/** @file

  Header of Function APIs for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFuncApi.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_GEN8_TS_FUNC_API_H_
#define _ELAN_GEN8_TS_FUNC_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "ElanTsFuncApi.h"

/***************************************************
 * Definitions
 ***************************************************/

// Remark ID Length
#ifndef ELAN_GEN8_REMARK_ID_LEN
#define	ELAN_GEN8_REMARK_ID_LEN		16
#endif //ELAN_GEN8_REMARK_ID_LEN

// Remark ID Index
#ifndef ELAN_GEN8_REMARK_ID_INDEX_ADDR
#define ELAN_GEN8_REMARK_ID_INDEX_ADDR	0x00042200
#endif //ELAN_GEN8_REMARK_ID_INDEX_ADDR

// Information ROM Address
#ifndef ELAN_GEN8_INFO_ROM_MEMORY_ADDR
#define	ELAN_GEN8_INFO_ROM_MEMORY_ADDR	0x00040000
#endif //ELAN_GEN8_INFO_ROM_MEMORY_ADDR

// Information Memory Page Address (The 4th Info. Memory Page)
#ifndef ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR
#define	ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR	0x00041800
#endif //ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR

// Memory Page Size
#ifndef ELAN_GEN8_MEMORY_PAGE_SIZE
#define ELAN_GEN8_MEMORY_PAGE_SIZE	2048  // 0x800
#endif //ELAN_GEN8_MEMORY_PAGE_SIZE

// Update Counter
#ifndef ELAN_GEN8_UPDATE_COUNTER_ADDR
#define ELAN_GEN8_UPDATE_COUNTER_ADDR			0x00041C00
#endif //ELAN_GEN8_UPDATE_COUNTER_ADDR

// Last Update Time (Year)
#ifndef ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR
#define ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR	0x00041C10
#endif //ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR

// Last Update Time (Month)
#ifndef ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR
#define ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR	0x00041C14
#endif //ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR

// Last Update Time (Day)
#ifndef ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR
#define ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR		0x00041C18
#endif //ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR

// Last Update Time (Hour)
#ifndef ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR
#define ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR	0x00041C1C
#endif //ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR

// Last Update Time (Minute)
#ifndef ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR
#define ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR	0x00041C20
#endif //ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR

/***************************************************
 * Macros
 ***************************************************/

/***************************************************
 * Declaration of Data Structure
 ***************************************************/

/***************************************************
 * Global Data Structure Declaration
 ***************************************************/

/***************************************************
 * Global Variables Declaration
 ***************************************************/

/***************************************************
 * Extern Variables Declaration
 ***************************************************/

/***************************************************
 * Function Prototype
 ***************************************************/

// Firmware Information
int gen8_get_test_version(unsigned short *p_test_version);

// IAP
int gen8_switch_to_boot_code(bool recovery);

// Erase Flash
int erase_flash_section(unsigned int address, unsigned short page_count);
int erase_flash(void);
int erase_info_page_flash(void);

// ROM Data
int gen8_get_rom_data(unsigned int addr, unsigned char data_len, unsigned int *p_data);

// Remark ID
int gen8_read_remark_id(unsigned char *p_gen8_remark_id_buf, size_t gen8_remark_id_buf_size, bool recovery);

// Memory / Firmware Page Data
int gen8_read_memory_page(unsigned short mem_page_address, unsigned short mem_page_size, unsigned char *p_mem_page_buf, size_t mem_page_buf_size);
int create_ektl_fw_page(unsigned int mem_page_address, unsigned char *p_ektl_fw_page_data_buf, size_t ektl_fw_page_data_buf_size, unsigned char *p_ektl_fw_page_buf, size_t ektl_fw_page_buf_size);
int write_ektl_fw_page(unsigned char *p_ektl_fw_page_buf, size_t ektl_fw_page_buf_size);

// Information Page
int gen8_get_info_page(unsigned char *p_info_page_buf, size_t info_page_buf_size);
int gen8_get_info_page_with_error_retry(unsigned char *p_info_page_buf, size_t info_page_buf_size, int retry_count);

int gen8_get_update_info(unsigned char *p_info_page_buf, size_t info_page_buf_size, struct update_info *p_update_info, size_t update_info_size);
int gen8_update_info_page(struct update_info *p_update_info, size_t update_info_size, unsigned char *p_info_page_buf, size_t info_page_buf_size);
int gen8_get_and_update_info_page(unsigned char *p_info_page_buf, size_t info_page_buf_size);

int gen8_set_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int value);
int gen8_set_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int value);

int gen8_get_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int *p_value);
int gen8_get_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int *p_value);

#endif //_ELAN_GEN8_TS_FUNC_API_H_
