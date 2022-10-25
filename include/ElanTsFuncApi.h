/** @file

  Header of Function APIs for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsFuncApi.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_FUNC_API_H_
#define _ELAN_TS_FUNC_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/***************************************************
 * Definitions
 ***************************************************/

// Information ROM Address
#ifndef ELAN_INFO_ROM_MEMORY_ADDR
#define	ELAN_INFO_ROM_MEMORY_ADDR	0x8000
#endif //ELAN_INFO_ROM_MEMORY_ADDR

// Information Memory Page Address (The 2nd Info. Memory Page)
#ifndef ELAN_INFO_MEMORY_PAGE_1_ADDR
#define	ELAN_INFO_MEMORY_PAGE_1_ADDR	0x8040
#endif //ELAN_INFO_MEMORY_PAGE_1_ADDR

// Information Page Address to Write
#ifndef ELAN_INFO_PAGE_WRITE_MEMORY_ADDR
#define	ELAN_INFO_PAGE_WRITE_MEMORY_ADDR	0x0040
#endif //ELAN_INFO_PAGE_WRITE_MEMORY_ADDR

// Update Counter
#ifndef ELAN_UPDATE_COUNTER_ADDR
#define ELAN_UPDATE_COUNTER_ADDR				0x8060
#endif //ELAN_UPDATE_COUNTER_ADDR

// Last Update Time (Year)
#ifndef ELAN_LAST_UPDATE_TIME_YEAR_ADDR
#define ELAN_LAST_UPDATE_TIME_YEAR_ADDR			0x8061
#endif //ELAN_LAST_UPDATE_TIME_YEAR_ADDR

// Last Update Time (Month_Day)
#ifndef ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR
#define ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR	0x8062
#endif //ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR

// Last Update Time (Hour_Minute)
#ifndef ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR
#define ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR	0x8063
#endif //ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR

// Information Page ROM FWID Address
#ifndef ELAN_INFO_ROM_FWID_MEMORY_ADDR
#define	ELAN_INFO_ROM_FWID_MEMORY_ADDR	    0x8080
#endif //ELAN_INFO_ROM_FWID_MEMORY_ADDR

// Elan ROM Address of Remark ID
#ifndef ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR
#define	ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR 0x801F
#endif //ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR

// Elan Remark ID of Non-Remark IC
#ifndef ELAN_REMARK_ID_OF_NON_REMARK_IC
#define	ELAN_REMARK_ID_OF_NON_REMARK_IC     0xFFFF
#endif //ELAN_REMARK_ID_OF_NON_REMARK_IC

// Memory Page Size
#ifndef ELAN_MEMORY_PAGE_SIZE
#define ELAN_MEMORY_PAGE_SIZE	128  // 0x40 (in word)
#endif //ELAN_MEMORY_PAGE_SIZE

// Error Retry Count
#ifndef ERROR_RETRY_COUNT
#define ERROR_RETRY_COUNT	3
#endif //ERROR_RETRY_COUNT

/***************************************************
 * Macros
 ***************************************************/

// High Byte
#ifndef HIGH_BYTE
#define HIGH_BYTE(data_word)    ((unsigned char)((data_word & 0xFF00) >> 8))
#endif //HIGH_BYTE

// Low Byte
#ifndef LOW_BYTE
#define LOW_BYTE(data_word)    ((unsigned char)(data_word & 0x00FF))
#endif //LOW_BYTE

/***************************************************
 * Declaration of Data Structure
 ***************************************************/

// Update Time
struct update_time
{
    unsigned int Year;
    unsigned int Month;
    unsigned int Day;
    unsigned int Hour;
    unsigned int Minute;
};
typedef struct update_time UPDATE_TIME, *P_UPDATE_TIME;

// Update Information
struct update_info
{
    unsigned int update_counter;
    struct update_time last_update_time;
};
typedef struct update_info UPDATE_INFO, *P_UPDATE_INFO;

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

/***************************************************
 * Function Prototype
 ***************************************************/

// Firmware Information
int get_boot_code_version(unsigned short *p_bc_version);
int get_firmware_id(unsigned short *p_fw_id);
int get_fw_version(unsigned short *p_fw_version);
int get_test_version(unsigned short *p_test_version);

// Solution ID
int get_solution_id(unsigned char *p_solution_id);

// Calibration
int calibrate_touch(void);
int calibrate_touch_with_error_retry(int retry_count);

// Calibration Counter
int get_rek_counter(unsigned short *p_rek_counter);

// Hello Packet / BC Version
int get_hello_packet_bc_version(unsigned char *p_hello_packet, unsigned short *p_bc_version);
int get_hello_packet_bc_version_with_error_retry(unsigned char *p_hello_packet, unsigned short *p_bc_version, int retry_count);
int get_hello_packet_with_error_retry(unsigned char *p_hello_packet, int retry_count);

// IAP
int switch_to_boot_code(bool recovery);
int check_slave_address(void);

// Remark ID
int read_remark_id(bool recovery);

// Memory / Firmware Page Data
int read_memory_page(unsigned short mem_page_address, unsigned short mem_page_size, unsigned char *p_mem_page_buf, size_t mem_page_buf_size);
int create_firmware_page(unsigned int mem_page_address, unsigned char *p_fw_page_data_buf, size_t fw_page_data_buf_size, unsigned char *p_fw_page_buf, size_t fw_page_buf_size);
int write_firmware_page(unsigned char *p_fw_page_buf, int fw_page_buf_size);

// Information Page
int get_info_page(unsigned char *info_page_buf, size_t info_page_buf_size);
int get_info_page_with_error_retry(unsigned char *info_page_buf, size_t info_page_buf_size, int retry_count);

int get_update_info(unsigned char *p_info_page_buf, size_t info_page_buf_size, struct update_info *p_update_info, size_t update_info_size);
int update_info_page(struct update_info *p_update_info, size_t update_info_size, unsigned char *p_info_page_buf, size_t info_page_buf_size);
int get_and_update_info_page(unsigned char solution_id, unsigned char *p_info_page_buf, size_t info_page_buf_size);

int set_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short value);
int set_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short value);
int set_info_page_data_bytes(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned char high_byte, unsigned char low_byte);

int get_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short *p_value);
int get_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short *p_value);
int get_info_page_data_bytes(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned char *p_high_byte, unsigned char *p_low_byte);

// Information FWID
int read_info_fwid(unsigned short *p_info_fwid, bool recovery);

// ROM Data
int get_rom_data(unsigned short addr, bool recovery, unsigned short *p_data);

// Bulk ROM Data
int get_bulk_rom_data(unsigned short addr, unsigned short *p_data);

#endif //_ELAN_TS_FUNC_API_H_
