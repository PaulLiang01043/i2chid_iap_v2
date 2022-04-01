/******************************************************************************
 * Copyright (c) 2019 ELAN Technology Corp. All Rights Reserved.
 *
 * Implementation of Elan I2C-HID Firmware Update Tool
 *
 *Release:
 *		2019/3
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "I2CHIDLinuxGet.h"
#include "ElanTsFuncApi.h"
#include "ElanTsIapFileIoUtility.h"

/***************************************************
 * Definitions
 ***************************************************/

// SW Version
#ifndef ELAN_TOOL_SW_VERSION
#define	ELAN_TOOL_SW_VERSION 	"3.1"
#endif //ELAN_TOOL_SW_VERSION

// SW Release Date
#ifndef ELAN_TOOL_SW_RELEASE_DATE
#define ELAN_TOOL_SW_RELEASE_DATE	"2022-03-31"
#endif //ELAN_TOOL_SW_RELEASE_DATE

// File Length
#ifndef FILE_NAME_LENGTH_MAX
#define FILE_NAME_LENGTH_MAX	256
#endif //FILE_NAME_LENGTH_MAX

#ifdef __SUPPORT_RESULT_LOG__
#ifndef DEFAULT_LOG_FILENAME
#define DEFAULT_LOG_FILENAME	"/tmp/elan_i2chid_iap_result.txt"
#endif //DEFAULT_LOG_FILENAME
#endif //__SUPPORT_RESULT_LOG__

/*
 * Action Code of Firmware Update
 */

// Remark ID Check
#ifndef ACTION_CODE_REMARK_ID_CHECK
#define ACTION_CODE_REMARK_ID_CHECK		0x01
#endif // ACTION_CODE_REMARK_ID_CHECK

// Update Information Section
#ifndef ACTION_CODE_INFORMATION_UPDATE
#define ACTION_CODE_INFORMATION_UPDATE	0x02
#endif // ACTION_CODE_INFORMATION_UPDATE

/*******************************************
 * Macros
 ******************************************/

#ifndef HIGH_BYTE
#define HIGH_BYTE(data_word)    ((unsigned char)((data_word & 0xFF00) >> 8))
#endif //HIGH_BYTE

#ifndef LOW_BYTE
#define LOW_BYTE(data_word)    ((unsigned char)(data_word & 0x00FF))
#endif //LOW_BYTE

/*******************************************
 * Global Variables Declaration
 ******************************************/

// Debug
bool g_debug = false;

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(fmt, argv...) if(g_debug) printf(fmt, ##argv)
#endif //DEBUG_PRINTF

#ifndef ERROR_PRINTF
#define ERROR_PRINTF(fmt, argv...) fprintf(stderr, fmt, ##argv)
#endif //ERROR_PRINTF

// InterfaceGet Class
CI2CHIDLinuxGet *g_pIntfGet = NULL;		// Pointer to I2CHID Inteface Class (CI2CHIDLinuxGet)

// PID
int g_pid = ELAN_USB_FORCE_CONNECT_PID;

// Flag for Firmware Update
bool g_update_fw = false;

// Firmware File Information
char g_firmware_filename[FILE_NAME_LENGTH_MAX] = {0};
int  g_firmware_fd = -1;
int  g_firmware_size = 0;

// Firmware Inforamtion
bool g_get_fw_info = false;

// Re-Calibration (Re-K)
bool g_rek = false;

// Skip Action Code
int g_skip_action_code = 0;

#ifdef __SUPPORT_RESULT_LOG__
// Result Log
char g_log_file[FILE_NAME_LENGTH_MAX] = {0};
#endif //__SUPPORT_RESULT_LOG__

// Silent Mode (Quiet)
bool g_quiet = false;

// Help Info.
bool g_help = false;

// BC Version
unsigned short g_bc_bc_version = 0,
               g_fw_bc_version = 0;

// FW Version
unsigned short g_fw_version = 0;

// Parameter Option Settings
#ifdef __SUPPORT_RESULT_LOG__
const char* const short_options = "p:P:f:s:oikl:qdh";
#else
const char* const short_options = "p:P:f:s:oikqdh";
#endif //__SUPPORT_RESULT_LOG__
const struct option long_options[] =
{
	{ "pid",					1, NULL, 'p'},
	{ "pid_hex",				1, NULL, 'P'},
	{ "file_path",				1, NULL, 'f'},
	{ "skip_action",			1, NULL, 's'},
	{ "firmware_information",	0, NULL, 'i'},
	{ "calibration",			0, NULL, 'k'},
#ifdef __SUPPORT_RESULT_LOG__
	{ "log_filename",			1, NULL, 'l'},
#endif //__SUPPORT_RESULT_LOG__
	{ "quiet",					0, NULL, 'q'},
    { "debug",					0, NULL, 'd'},
	{ "help",					0, NULL, 'h'},
};

/*******************************************
 * Function Prototype
 ******************************************/

// Firmware Information
int get_firmware_information(bool quiet /* Silent Mode */);

// Hello Packet
int get_hello_packet_with_error_retry(unsigned char *p_hello_packet, int retry_count);

// ROM Data
int get_rom_data(unsigned short addr, bool recovery, unsigned short *p_data);

// Remark ID Check
int check_remark_id(bool recovery);
int read_remark_id(bool recovery);

// Update Firmware
int update_firmware(char *filename, size_t filename_len, bool recovery);

#ifdef __SUPPORT_RESULT_LOG__
// Result Log
int generate_result_log(char *filename, size_t filename_len, bool result); 
#endif //__SUPPORT_RESULT_LOG__

// Help
void show_help_information(void);

// HID Raw I/O Function
int __hidraw_write(unsigned char* buf, int len, int timeout_ms); 
int __hidraw_read(unsigned char* buf, int len, int timeout_ms);

// Abstract Device I/O Function
int write_cmd(unsigned char *cmd_buf, int len, int timeout_ms);
int read_data(unsigned char *data_buf, int len, int timeout_ms);
int write_vendor_cmd(unsigned char *cmd_buf, int len, int timeout_ms);
int open_device(void);
int close_device(void);

// Default Function
int process_parameter(int argc, char **argv);
int resource_init(void);
int resource_free(void);
int main(int argc, char **argv);

/*******************************************
 * HID Raw I/O Functions
 ******************************************/

int __hidraw_write(unsigned char* buf, int len, int timeout_ms)
{
	int nRet = TP_SUCCESS;

	if(g_pIntfGet == NULL)
	{
		nRet = TP_ERR_COMMAND_NOT_SUPPORT;
		goto __HIDRAW_WRITE_EXIT;
	}

	nRet = g_pIntfGet->WriteRawBytes(buf, len, timeout_ms);

__HIDRAW_WRITE_EXIT:
	return nRet;
}

int __hidraw_read(unsigned char* buf, int len, int timeout_ms)
{
	int nRet = TP_SUCCESS;

	if(g_pIntfGet == NULL)
	{
		nRet = TP_ERR_COMMAND_NOT_SUPPORT;
		goto __HIDRAW_READ_EXIT;
	}

	nRet = g_pIntfGet->ReadRawBytes(buf, len, timeout_ms);

__HIDRAW_READ_EXIT:
	return nRet;
}

int __hidraw_write_command(unsigned char* buf, int len, int timeout_ms)
{
	int nRet = TP_SUCCESS;

	if(g_pIntfGet == NULL)
	{
		nRet = TP_ERR_COMMAND_NOT_SUPPORT;
		goto __HIDRAW_WRITE_EXIT;
	}

	nRet = g_pIntfGet->WriteCommand(buf, len, timeout_ms);

__HIDRAW_WRITE_EXIT:
	return nRet;
}

int __hidraw_read_data(unsigned char* buf, int len, int timeout_ms)
{
	int nRet = TP_SUCCESS;

	if(g_pIntfGet == NULL)
	{
		nRet = TP_ERR_COMMAND_NOT_SUPPORT;
		goto __HIDRAW_READ_EXIT;
	}

	nRet = g_pIntfGet->ReadData(buf, len, timeout_ms);

__HIDRAW_READ_EXIT:
	return nRet;
}

/***************************************************
 * Abstract I/O Functions
 ***************************************************/

int write_cmd(unsigned char *cmd_buf, int len, int timeout_ms)
{
    //write_bytes_from_buffer_to_i2c(cmd_buf); //pseudo function

    /*** example *********************/
    return __hidraw_write_command(cmd_buf, len, timeout_ms);
    /*********************************/
}

int read_data(unsigned char *data_buf, int len, int timeout_ms)
{
    //read_bytes_from_i2c_to_buffer(data_buf, len, timeout); //pseudo function

    /*** example *********************/
    return __hidraw_read_data(data_buf, len, timeout_ms);
    /*********************************/
}

int write_vendor_cmd(unsigned char *cmd_buf, int len, int timeout_ms)
{
	unsigned char vendor_cmd_buf[ELAN_I2CHID_OUTPUT_BUFFER_SIZE] = {0};

	// Add HID Header
	vendor_cmd_buf[0] = ELAN_HID_OUTPUT_REPORT_ID;
	memcpy(&vendor_cmd_buf[1], cmd_buf, len);

	return __hidraw_write(vendor_cmd_buf, sizeof(vendor_cmd_buf), timeout_ms);
}

/*******************************************
 * Function Implementation
 ******************************************/

int get_firmware_information(bool quiet /* Silent Mode */)
{
	int err = TP_SUCCESS;
    unsigned short fw_id = 0,
                   fw_version = 0,
                   test_version = 0,
                   bc_version = 0;

	if(quiet == true) // Enable Silent Mode
	{
		// Firmware Version
        err = get_fw_version(&fw_version);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
        printf("%04x", fw_version);
	}
	else // Disable Silent Mode
	{
		printf("--------------------------------\r\n");

		// FW ID
        err = get_firmware_id(&fw_id);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
        printf("Firmware ID: %02x.%02x\r\n", HIGH_BYTE(fw_id), LOW_BYTE(fw_id));

		// Firmware Version
        err = get_fw_version(&fw_version);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
        printf("Firmware Version: %02x.%02x\r\n", HIGH_BYTE(fw_version), LOW_BYTE(fw_version));
        g_fw_version = fw_version;

		// Test Version
        err = get_test_version(&test_version);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
        printf("Test Version: %02x.%02x\r\n", HIGH_BYTE(test_version), LOW_BYTE(test_version));

		// Boot Code Version
        err = get_boot_code_version(&bc_version);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
        printf("Boot Code Version: %02x.%02x\r\n", HIGH_BYTE(bc_version), LOW_BYTE(bc_version));
        g_fw_bc_version = bc_version;
	}

GET_FW_INFO_EXIT:
	return err;
}

// ROM Data
int get_rom_data(unsigned short addr, bool recovery, unsigned short *p_data)
{
	int err = TP_SUCCESS;
    unsigned short word_data = 0;
    unsigned char  solution_id = 0,
                   bc_version_high_byte = 0;

	// Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GET_ROM_DATA_EXIT;
    }

    // Get Solution ID & High Byte of Boot Code
    solution_id = HIGH_BYTE(g_fw_version);
    bc_version_high_byte = HIGH_BYTE(g_bc_bc_version);
    DEBUG_PRINTF("Solution ID: %02x, High Byte of Boot Code Version: %02x.\r\n", solution_id, bc_version_high_byte);

    /* Read Data from ROM */
    err = send_read_rom_data_command(addr, recovery, (recovery) ? bc_version_high_byte : solution_id);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Read ROM Data Command! errno=0x%x.\r\n", __func__, err);
        goto GET_ROM_DATA_EXIT;
    }

    err = receive_rom_data(&word_data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive ROM Data! errno=0x%x.\r\n", __func__, err);
        goto GET_ROM_DATA_EXIT;
    }

    *p_data = word_data;
    err = TP_SUCCESS;

GET_ROM_DATA_EXIT:
	return err; 
}

int check_remark_id(bool recovery)
{
	int err = TP_SUCCESS;
	unsigned short remark_id_from_rom = 0,
                   remark_id_from_fw  = 0;

    // Get Remark ID from ROM
    err = get_rom_data(ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR, recovery, &remark_id_from_rom);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Remark ID from ROM! errno=0x%x.\r\n", __func__, err);
        goto CHECK_REMARK_ID_EXIT;
    }

    // Read Remark ID from Firmware
    err = get_remark_id_from_firmware(&remark_id_from_fw);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Remark ID from Firmware! errno=0x%x.\r\n", __func__, err);
        goto CHECK_REMARK_ID_EXIT;
    }

    DEBUG_PRINTF("Remark ID from ROM: %04x, Remark ID from FW: %04x.\r\n", remark_id_from_rom, remark_id_from_fw);

    // Just Pass if Non-Remark IC
    if(remark_id_from_rom == ELAN_REMARK_ID_OF_NON_REMARK_IC)
    {
        err = TP_SUCCESS;
        goto CHECK_REMARK_ID_EXIT;
    }
    else // Remark IC
    {
        // Validate Remark ID 
	    if(remark_id_from_rom != remark_id_from_fw) 
	    {
            ERROR_PRINTF("%s: Remark ID Mismatched! (Remark ID from ROM: %04x, Remark ID from FW: %04x)\r\n", __func__, remark_id_from_rom, remark_id_from_fw);
		    err = TP_ERR_DATA_MISMATCHED;
            goto CHECK_REMARK_ID_EXIT;
	    }
    }

    err = TP_SUCCESS;

CHECK_REMARK_ID_EXIT:
	return err;
}

int read_remark_id(bool recovery)
{
	int err = TP_SUCCESS;
	unsigned short remark_id = 0;

	// Get Remark ID from ROM
    err = get_rom_data(ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR, recovery, &remark_id);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Read Remark ID from ROM! errno=0x%x.\r\n", __func__, err);
        goto READ_REMARK_ID_EXIT;
    }
	DEBUG_PRINTF("Remark ID: %04x.\r\n", remark_id);

    err = TP_SUCCESS;

READ_REMARK_ID_EXIT:
	return err;	
}

int get_hello_packet_with_error_retry(unsigned char *p_hello_packet, int retry_count)
{
	int err = TP_SUCCESS,
		retry_index = 0;

	// Make Sure Page Data Buffer Valid
	if(p_hello_packet == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_HELLO_PACKET_WITH_ERROR_RETRY_EXIT;
	}

	// Make Sure Retry Count Positive
	if(retry_count <= 0)
		retry_count = 1;

	for(retry_index = 0; retry_index < retry_count; retry_index++)
	{
        err = get_hello_packet_bc_version(p_hello_packet, &g_bc_bc_version);
		if(err == TP_SUCCESS)
		{
			// Without any error => Break retry loop and continue.
			break;
		}

		// With Error => Retry at most 3 times 
		DEBUG_PRINTF("%s: [%d/3] Fail to Get Hello Packet! errno=0x%x.\r\n", __func__, retry_index+1, err);
		if(retry_index == 2)
		{
			// Have retried for 3 times and can't fix it => Stop this function 
			ERROR_PRINTF("%s: Fail to Get Hello Packet! errno=0x%x.\r\n", __func__, err);
			goto GET_HELLO_PACKET_WITH_ERROR_RETRY_EXIT;
		}
		else // retry_index = 0, 1
		{
			// wait 50ms
			usleep(50*1000); 

			continue;
		}		
	}

GET_HELLO_PACKET_WITH_ERROR_RETRY_EXIT:
	return err;
}

int update_firmware(char *filename, size_t filename_len, bool recovery)
{
	int err = TP_SUCCESS,
		page_count = 0,
		block_count = 0,
		block_index = 0,
		block_page_num = 0;
	unsigned char info_page_buf[ELAN_FIRMWARE_PAGE_SIZE] = {0},
				  page_block_buf[ELAN_FIRMWARE_PAGE_SIZE * 30] = {0},
                  bc_ver_high_byte = 0,
                  bc_ver_low_byte = 0,
                  iap_version = 0,
				  solution_id = 0;
    bool remark_id_check = false,
		 skip_remark_id_check = false,
		 skip_information_update = false;
#if defined(__ENABLE_DEBUG__) && defined(__ENABLE_SYSLOG_DEBUG__)
    bool bDisableOutputBufferDebug = false;
#endif //__ENABLE_SYSLOG_DEBUG__ && __ENABLE_SYSLOG_DEBUG__

	// Make Sure Filename Valid
	if(filename == NULL)
	{
		ERROR_PRINTF("%s: Null String of Filename!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto UPDATE_FIRMWARE_EXIT;
	}

	// Make Sure Filename Length Valid
	if(filename_len == 0)
	{
		ERROR_PRINTF("%s: Filename String Length is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto UPDATE_FIRMWARE_EXIT;
	}

	// Make Sure File Exist
	if(access(filename, F_OK) == -1)
	{
		ERROR_PRINTF("%s: File \"%s\" does not exist!\r\n", __func__, filename);
		err = TP_ERR_FILE_NOT_FOUND;
		goto UPDATE_FIRMWARE_EXIT;
	}

	printf("--------------------------------\r\n");
	printf("FW Path: \"%s\".\r\n", filename);

	// Set Global Flag of Skip Action Code
	if((g_skip_action_code & ACTION_CODE_REMARK_ID_CHECK) == ACTION_CODE_REMARK_ID_CHECK)
		skip_remark_id_check = true;
	if((g_skip_action_code & ACTION_CODE_INFORMATION_UPDATE) == ACTION_CODE_INFORMATION_UPDATE)
		skip_information_update = true;
	DEBUG_PRINTF("skip_remark_id_check: %s, skip_information_update: %s.\r\n", \
										(skip_remark_id_check) ? "true" : "false", \
										(skip_information_update) ? "true" : "false");
	
	if((recovery == false) && (skip_information_update == false)) // Normal Mode & Don't Skip Information (Section) Update
	{
		// Get Solution ID
		solution_id = HIGH_BYTE(g_fw_version); 	// Only Needed in Normal Mode

		//
		// Get & Update Information Page
		//
		err = get_and_update_info_page(solution_id, info_page_buf, sizeof(info_page_buf));
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to get/update Inforamtion Page! errno=0x%x.\r\n", __func__, err);
			goto UPDATE_FIRMWARE_EXIT;
		}
	}

    //
    // Remark ID Check 
    //
    if(recovery == false) // Normal Mode
    {
        iap_version = (unsigned char)(g_fw_bc_version & 0x00FF);
		if(iap_version >= 0x60)
			remark_id_check = true;
		DEBUG_PRINTF("BC Version: %04x, remark_id_check: %s.\r\n", g_fw_bc_version, (remark_id_check) ? "true" : "false");
    }
    else // Recovery Mode
    {
        bc_ver_high_byte = (unsigned char)((g_bc_bc_version & 0xFF00) >> 8);
        bc_ver_low_byte  = (unsigned char)(g_bc_bc_version & 0x00FF);
       	if(bc_ver_high_byte != bc_ver_low_byte) // EX: A7 60
            remark_id_check = true;
		DEBUG_PRINTF("BC Version: %04x, remark_id_check: %s.\r\n", g_bc_bc_version, (remark_id_check) ? "true" : "false");
    }
    if(remark_id_check == true)
	{
		if(skip_remark_id_check == false) // Check Remark ID
		{
        	DEBUG_PRINTF("[%s Mode] Check Remark ID...\r\n", (recovery) ? "Recovery" : "Normal");

        	err = check_remark_id(recovery);
        	if(err != TP_SUCCESS)
			{
				ERROR_PRINTF("%s: Remark ID Check Failed! errno=0x%x.\r\n", __func__, err);
				goto UPDATE_FIRMWARE_EXIT;
			}
		}
		else // Skip Reamrk ID Check, but read Remark ID
		{
			DEBUG_PRINTF("[%s Mode] Read Remark ID...\r\n", (recovery) ? "Recovery" : "Normal");
			
			err = read_remark_id(recovery);
        	if(err != TP_SUCCESS)
			{
				ERROR_PRINTF("%s: Read Remark ID Failed! errno=0x%x.\r\n", __func__, err);
				goto UPDATE_FIRMWARE_EXIT;
			}
		}
	}

	//
	// Switch to Boot Code
	//
	//err = switch_to_boot_code();
    err = switch_to_boot_code(recovery);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to switch to Boot Code! errno=0x%x.\r\n", __func__, err);
		goto UPDATE_FIRMWARE_EXIT;
	}

	printf("Start FW Update Process...\r\n");

	// 
	// Update with FW Pages 
	//

#if defined(__ENABLE_DEBUG__) && defined(__ENABLE_SYSLOG_DEBUG__)
    if((g_bEnableOutputBufferDebug == true) && (g_debug == false))
    {
        // Disable Output Buffer Debug
        DEBUG_PRINTF("Disable Output Buffer Debug.\r\n");
        g_bEnableOutputBufferDebug = false;
        bDisableOutputBufferDebug = true;
    }
#endif //__ENABLE_SYSLOG_DEBUG__ && __ENABLE_SYSLOG_DEBUG__

	if((recovery == false) && (skip_information_update == false)) // Normal Mode & Don't Skip Information (Section) Update
	{
		// Write Information Page
		DEBUG_PRINTF("Update Information Page...\r\n");
		err = write_page_data(info_page_buf, sizeof(info_page_buf));
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to Write Infomation Page! errno=0x%x.\r\n", __func__, err);
			goto UPDATE_FIRMWARE_EXIT;
		}
	}

    // Reset file pointer
    lseek(g_firmware_fd, 0, SEEK_SET); 

	// Write Main Pages
	page_count = compute_firmware_page_number(g_firmware_size);
	block_count = (page_count / 30) + ((page_count % 30) != 0);
	DEBUG_PRINTF("Update %d Main Pages with %d Page Blocks...\r\n", page_count, block_count);
	for(block_index = 0; block_index < block_count; block_index++)
	{
		// Print test progress to inform operators
        printf(".");
        fflush(stdout);

		// Clear Page Block Buffer
		memset(page_block_buf, 0, sizeof(page_block_buf));

		// Get Bulk FW Page Data
		if((block_index == (block_count - 1)) && ((page_count % 30) != 0)) // Last Block
			block_page_num = page_count % 30; // Last Block Page Number
		else
			block_page_num = 30; // 30 Page

		// Load Page Data into Buffer
		err = retrieve_data_from_firmware(page_block_buf, ELAN_FIRMWARE_PAGE_SIZE * block_page_num);
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to Retrieve Page Block Data from Firmware! errno=0x%x.\r\n", __func__, err);
			goto UPDATE_FIRMWARE_EXIT;
		}

		// Write Bulk FW Page Data
		err = write_page_data(page_block_buf, ELAN_FIRMWARE_PAGE_SIZE * block_page_num);
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to Write FW Page Block %d (%d-Page)! errno=0x%x.\r\n", __func__, block_index, block_page_num, err);
			goto UPDATE_FIRMWARE_EXIT;
		}
	}

	// 
	// Self-Reset
	//
	sleep(1); // wait for 1s
	printf("\r\n"); //Print CRLF in console

	// Success
	printf("FW Update Finished.\r\n");
	err = TP_SUCCESS;

UPDATE_FIRMWARE_EXIT:

#if defined(__ENABLE_DEBUG__) && defined(__ENABLE_SYSLOG_DEBUG__)
    if(bDisableOutputBufferDebug == true)
    {
        // Re-Enable Output Buffer Debug
        DEBUG_PRINTF("Re-Enable Output Buffer Debug.\r\n");
        g_bEnableOutputBufferDebug = true;
    }
#endif //__ENABLE_SYSLOG_DEBUG__ && __ENABLE_SYSLOG_DEBUG__

	return err;
}

/*******************************************
 * Log
 ******************************************/

#ifdef __SUPPORT_RESULT_LOG__
int generate_result_log(char *filename, size_t filename_len, bool result)
{
	int err = TP_SUCCESS;
    FILE *fd = NULL;

	if(g_quiet == false) // Disable Silent Mode
		printf("--------------------------------\r\n");

	// Make Sure Filename Valid
	if(filename == NULL)
	{
		ERROR_PRINTF("%s: Null String of Filename!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GENERATE_RESULT_LOG_EXIT;
	}

	// Make Sure Filename Length Valid
	if(filename_len == 0)
	{
		ERROR_PRINTF("%s: Filename String Length is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GENERATE_RESULT_LOG_EXIT;
	}
	
	// Debug
	DEBUG_PRINTF("%s: filename=\"%s\", filename_len=%zd, result=%s.\r\n", __func__, filename, filename_len, (result) ? "true" : "false");

    // Remove Old Log File
    remove(filename);

    // Create a new file
    fd = fopen(filename, "w");
    if (fd == NULL)
	{
		ERROR_PRINTF("%s: Fail to open log file \"%s\"!\r\n", __func__, filename);
		err = TP_ERR_FILE_NOT_FOUND;
		goto GENERATE_RESULT_LOG_EXIT;
	}

    // Write result to file
    if (result == true) // Pass
        fprintf(fd, "PASS\n");
    else // Fail
        fprintf(fd, "FAIL\n");

    fclose(fd);

GENERATE_RESULT_LOG_EXIT:
	return err;
}
#endif //__SUPPORT_RESULT_LOG__

/*******************************************
 * Help
 ******************************************/

void show_help_information(void)
{
	printf("--------------------------------\r\n");
	printf("SYNOPSIS:\r\n");

	// PID
	printf("\n[PID]\r\n");
	printf("-p <pid in decimal>.\r\n");
	printf("Ex: elan_iap -p 1842\r\n");
	printf("-P <PID in hex>.\r\n");
	printf("Ex: elan_iap -P 732 (0x732)\r\n");

	// File Path
	printf("\n[File Path]\r\n");
	printf("-f <file_path>.\r\n");
	printf("Ex: elan_iap -f firmware.ekt\r\n");
	printf("Ex: elan_iap -f /tmp/firmware.ekt\r\n");

	// Skip Action
	printf("\n[Skip Action]\r\n");
	printf("-s <action_code>.\r\n");
	printf("Ex: elan_iap -s 1 \r\n");

	// Firmware Information
	printf("\n[Firmware Information]\r\n");
	printf("-i.\r\n");
	printf("Ex: elan_iap -i\r\n");

	// Re-Calibarion
	printf("\n[Re-Calibarion]\r\n");
	printf("-k.\r\n");
	printf("Ex: elan_iap -k\r\n");

#ifdef __SUPPORT_RESULT_LOG__
	// Result Log
	printf("\n[Result Log File Path]\r\n");
	printf("-l <result_log_file_path>.\r\n");
	printf("Ex: elan_iap -l result.txt\r\n");
	printf("Ex: elan_iap -l /tmp/result.txt\r\n");
#endif //__SUPPORT_RESULT_LOG__

	// Silent (Quiet) Mode
	printf("\n[Silent Mode]\r\n");
	printf("-q.\r\n");
	printf("Ex: elan_iap -q\r\n");

	// Debug Information
	printf("\n[Debug]\r\n");
	printf("-d.\r\n");
	printf("Ex: elan_iap -d\r\n");

	// Help Information
	printf("\n[Help]\r\n");
	printf("-h.\r\n");
	printf("Ex: elan_iap -h\r\n");

	return;
}

/*******************************************
 *  Open & Close Device
 ******************************************/

int open_device(void)
{
    int err = TP_SUCCESS;

    // open specific device on i2c bus //pseudo function

    /*** example *********************/
    // Connect to Device
    DEBUG_PRINTF("Get I2C-HID Device Handle (VID=0x%x, PID=0x%x).\r\n", ELAN_USB_VID, g_pid);
    err = g_pIntfGet->GetDeviceHandle(ELAN_USB_VID, g_pid);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Device can't connected! errno=0x%x.\n", err);
    /*********************************/

    return err;
}

int close_device(void)
{
    int err = 0;

    // close opened i2c device; //pseudo function

    /*** example *********************/
    // Release acquired touch device handler
    g_pIntfGet->Close();
    /*********************************/

    return err;
}

/*******************************************
 *  Initialize & Free Resource
 ******************************************/

int resource_init(void)
{
    int err = TP_SUCCESS;

    //initialize_resource(); //pseudo function

    /*** example *********************/
    // Initialize Interface
    g_pIntfGet = new CI2CHIDLinuxGet();
    DEBUG_PRINTF("g_pIntfGet=%p.\n", g_pIntfGet);
    if (g_pIntfGet == NULL)
    {
        ERROR_PRINTF("Fail to initialize I2C-HID Interface!");
        err = TP_ERR_NO_INTERFACE_CREATE;
		goto RESOURCE_INIT_EXIT;
    }

	if(g_update_fw == true)
	{
		// Open Firmware File
		err = open_firmware_file(g_firmware_filename, strlen(g_firmware_filename), &g_firmware_fd);
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("Fail to open firmware file \"%s\"! errno=0x%x.\r\n", g_firmware_filename, errno);
			goto RESOURCE_INIT_EXIT;
		}
	
		// Get Firmware Size
		err = get_firmware_size(g_firmware_fd, &g_firmware_size);
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("Fail to open firmware file \"%s\"! errno=0x%x.\r\n", g_firmware_filename, errno);
			goto RESOURCE_INIT_EXIT;
		}

		// Make Sure Firmware File Valid
		//DEBUG_PRINTF("Firmware fd=%d, size=%d.\r\n", g_firmware_fd, g_firmware_size);
		if((g_firmware_fd < 0) || (g_firmware_size <= 0))
		{
			ERROR_PRINTF("Fail to open firmware file \'%s\', size=%d, errno=0x%x.\r\n", \
              g_firmware_filename, g_firmware_size, g_firmware_fd);
			err = TP_ERR_FILE_NOT_FOUND;
			goto RESOURCE_INIT_EXIT;
		}
		lseek(g_firmware_fd, 0, SEEK_SET); // Reset file pointer
	}
    /*********************************/

RESOURCE_INIT_EXIT:
    return err;
}

int resource_free(void)
{
    int err = TP_SUCCESS;

    //release_resource(); //pseudo function

    /*** example *********************/
	if(g_update_fw == true)
	{
		// Close Firmware File
		close_firmware_file(g_firmware_fd);
		g_firmware_fd = -1;
	}

    // Release Interface
    if (g_pIntfGet)
    {
        delete dynamic_cast<CI2CHIDLinuxGet *>(g_pIntfGet);
        g_pIntfGet = NULL;
    }
    /*********************************/

    return err;
}

/***************************************************
* Parser command
***************************************************/

int process_parameter(int argc, char **argv)
{
    int err = TP_SUCCESS,
		opt = 0,
        option_index = 0,
		pid = 0,
		pid_str_len = 0,
		file_path_len = 0,
		action_code = 0;
	char file_path[FILE_NAME_LENGTH_MAX] = {0};

    while (1)
    {
        opt = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (opt == EOF)	break;

        switch (opt)
        {
            case 'p': /* PID (Decimal) */

                // Make Sure Data Valid
                pid = atoi(optarg);
                if (pid < 0)
                {
                    ERROR_PRINTF("%s: Invalid PID: %d!\n", __func__, pid);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Set Global ADC Type
                g_pid = pid;
				DEBUG_PRINTF("%s: PID=%d(0x%x).\r\n", __func__, g_pid, g_pid);
                break;

			case 'P': /* PID (Hex) */

                // Make Sure Format Valid
                pid_str_len = strlen(optarg);
                if (pid_str_len > 4)
                {
                    ERROR_PRINTF("%s: Invalid String Length for PID: %d!\n", __func__, pid_str_len);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

				// Make Sure Data Valid
                pid = strtol(optarg, NULL, 16);
                if (pid < 0)
                {
                    ERROR_PRINTF("%s: Invalid PID: %d!\n", __func__, pid);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Set Global ADC Type
                g_pid = pid;
				DEBUG_PRINTF("%s: PID=0x%x.\r\n", __func__, g_pid);
                break;

            case 'f': /* FW File Path */

                // Check if filename is valid
                file_path_len = strlen(optarg);
                if ((file_path_len == 0) || (file_path_len > FILE_NAME_LENGTH_MAX))
                {
                    ERROR_PRINTF("%s: Firmware Path (%s) Invalid!\r\n", __func__, optarg);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

				// Check if file path is valid
				strcpy(file_path, optarg);
                //DEBUG_PRINTF("%s: fw file path=\"%s\".\r\n", __func__, file_path);
				if(strncmp(file_path, "", strlen(file_path)) == 0)
				{
                    ERROR_PRINTF("%s: NULL Firmware Path!\r\n", __func__);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

				// Set FW Update Flag
				g_update_fw = true;

				// Set Global File Path
                strncpy(g_firmware_filename, file_path, strlen(file_path));
				DEBUG_PRINTF("%s: Update FW: %s, File Path: \"%s\".\r\n", __func__, (g_update_fw) ? "Yes" : "No", g_firmware_filename);
                break;

			case 's': /* Skip Action */

                // Make Sure Data Valid
                action_code = atoi(optarg);
                if (action_code < 0)
                {
                    ERROR_PRINTF("%s: Invalid Action Code: %d!\n", __func__, action_code);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Set Global ADC Type
                g_skip_action_code = action_code;
				DEBUG_PRINTF("%s: Skip Action Code: %d.\r\n", __func__, g_skip_action_code);
                break;

			case 'i': /* Firmware Information */

                // Set "Get FW Info." Flag
                g_get_fw_info = true;
                DEBUG_PRINTF("%s: Get FW Inforamtion: %s.\r\n", __func__, (g_get_fw_info) ? "Enable" : "Disable");
                break;

			case 'k': /* Re-Calibration (Re-K) */

                // Set Re-K Flag
                g_rek = true;
                DEBUG_PRINTF("%s: Re-Calibration: %s.\r\n", __func__, (g_rek) ? "Enable" : "Disable");
                break;

#ifdef __SUPPORT_RESULT_LOG__
            case 'l': /* Log Filename */

                 // Check if filename is valid
                file_path_len = strlen(optarg);
                if ((file_path_len == 0) || (file_path_len > FILE_NAME_LENGTH_MAX))
                {
                    ERROR_PRINTF("%s: Log Path (%s) Invalid!\r\n", __func__, optarg);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

				// Check if file path is valid
				strcpy(file_path, optarg);
				if(strncmp(file_path, "", strlen(file_path)) == 0)
				{
                    ERROR_PRINTF("%s: NULL Log Path!\r\n", __func__);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Set log filename
                strncpy(g_log_file, optarg, file_path_len);
                DEBUG_PRINTF("%s: Log Filename: \"%s\".\r\n", __func__, g_log_file);

                // Remove Content of Output Data File
                remove(g_log_file);
                break;
#endif //__SUPPORT_RESULT_LOG__

			case 'q': /* Silent Mode (Quiet) */

                // Enable Silent Mode (Quiet)
                g_quiet = true;
                DEBUG_PRINTF("%s: Silent Mode: %s.\r\n", __func__, (g_quiet) ? "Enable" : "Disable");
                break;

            case 'd': /* Debug Option */

                // Enable Debug & Output Buffer Debug
                g_debug = true;
                DEBUG_PRINTF("Debug: %s.\r\n", (g_debug) ? "Enable" : "Disable");
                break;

			case 'h': /* Help */

                // Set debug
                g_help = true;
                DEBUG_PRINTF("Help Information: %s.\r\n", (g_help) ? "Enable" : "Disable");
                break;

            default:
                ERROR_PRINTF("%s: Unknown Command!\r\n", __func__);
                break;
        }
    }

	// Check if PID is not null
	if(g_pid == 0)
	{
		DEBUG_PRINTF("%s: PID is not set, look for an appropriate PID...\r\n", __func__);
	}

#ifdef __SUPPORT_RESULT_LOG__
	// Check if log filename is not null
    if (strcmp(g_log_file, "") == 0)
    {
        DEBUG_PRINTF("%s: Log filename is not set, set as \"%s\".\r\n", __func__, DEFAULT_LOG_FILENAME);

        // Set log filename
        strncpy(g_log_file, DEFAULT_LOG_FILENAME, sizeof(DEFAULT_LOG_FILENAME));
        DEBUG_PRINTF("%s: Default Log Filename: \"%s\".\r\n", __func__, g_log_file);

        // Remove Content of Output Data File
        remove(g_log_file);
    }
#endif //__SUPPORT_RESULT_LOG__

    return TP_SUCCESS;

PROCESS_PARAM_EXIT:
    DEBUG_PRINTF("[ELAN] ParserCmd: Exit because of an error occurred, errno=0x%x.\r\n", err);
    return err;
}

/*******************************************
 * Main Function
 ******************************************/

int main(int argc, char **argv)
{
    int err = TP_SUCCESS;
	unsigned char hello_packet = 0;
	bool recovery = false; // Recovery Mode

	// Process Parameter
	err = process_parameter(argc, argv);
    if (err != TP_SUCCESS)
	{
        goto EXIT;
	}

	if (g_quiet == false) // Disable Silent Mode
	{
		printf("i2chid_iap v%s %s.\r\n", ELAN_TOOL_SW_VERSION , ELAN_TOOL_SW_RELEASE_DATE);
	}

	/* Show Help Information */
	if(g_help == true)
	{
		show_help_information();
		goto EXIT;
	}

    /* Initialize Resource */
	err = resource_init();
    if (err != TP_SUCCESS)
	{
		ERROR_PRINTF("Fail to Init Resource! errno=0x%x.\r\n", err);
        goto EXIT1;
	}

    /* Open Device */
	err = open_device() ;
    if (err != TP_SUCCESS)
	{
		ERROR_PRINTF("Fail to Open Device! errno=0x%x.\r\n", err);
        goto EXIT2;
	}

	/* Check for Recovery */
	err = get_hello_packet_with_error_retry(&hello_packet, ERROR_RETRY_COUNT);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("Fail to Get Hello Packet! errno=0x%x.\r\n", err);
		goto EXIT2;
	}

	// Reconfigure if Recovery Mode
	if(hello_packet == ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET)
	{
		printf("In Recovery Mode.\r\n");
		recovery = true; 		// Enable Recovery
		g_get_fw_info = false;	// Disable Get FW Info.
		g_rek = false;			// Disable Re-Calibration
	}

	/* Get FW Information */
	if((g_get_fw_info == true) && (g_update_fw == false))
	{
    	DEBUG_PRINTF("Get FW Info.\r\n");
		err = get_firmware_information(g_quiet /* Slient Mode */);
		if(err != TP_SUCCESS)
		{
        	ERROR_PRINTF("Fail to Get FW Info!\r\n");
        	goto EXIT2;
    	}
	}

	/* Re-calibrate Touch */
	if(g_rek == true)
	{
		DEBUG_PRINTF("Calibrate Touch...\r\n");
		//err = calibrate_touch() ;
        err = calibrate_touch_with_error_retry(ERROR_RETRY_COUNT);
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("Fail to Calibrate Touch!\r\n");
			goto EXIT2;
    	}
	}

	/* Update FW */
	if(g_update_fw == true)
	{
		if(recovery == false) // Normal IAP
		{
			// Get FW Info.
			DEBUG_PRINTF("Get FW Info.\r\n");
			err = get_firmware_information(false /* Disable Silent Mode */);
			if(err != TP_SUCCESS)
			{
        		ERROR_PRINTF("Fail to Get FW Info!\r\n");
        		goto EXIT2;
    		}
		}

		// Update Firmware
		DEBUG_PRINTF("Update Firmware (%s).\r\n", g_firmware_filename);
		err = update_firmware(g_firmware_filename, strlen(g_firmware_filename), recovery);
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("Fail to Update Firmware (%s)!\r\n", g_firmware_filename);
			goto EXIT2;
    	}

		// Re-calibrate Touch
		DEBUG_PRINTF("Calibrate Touch...\r\n");
		//err = calibrate_touch() ;
        err = calibrate_touch_with_error_retry(ERROR_RETRY_COUNT);
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("Fail to Calibrate Touch!\r\n");
			goto EXIT2;
    	}

		// Get FW Info.
		DEBUG_PRINTF("Get FW Info.\r\n");
		err = get_firmware_information(false /* Disable Silent Mode */);
		if(err != TP_SUCCESS)
		{
        	ERROR_PRINTF("Fail to Get FW Info!\r\n");
        	goto EXIT2;
    	}
	}

	// Success
    err = TP_SUCCESS;

EXIT2:
    /* Close Device */
    close_device();

EXIT1:
    resource_free();

EXIT:
#ifdef __SUPPORT_RESULT_LOG__
	// Log Result
	if(err == TP_SUCCESS)
		generate_result_log(g_log_file, sizeof(g_log_file), true /* PASS */);
	else
		generate_result_log(g_log_file, sizeof(g_log_file), false /* FAIL */);
#endif //__SUPPORT_RESULT_LOG__

	if(g_quiet == false) // Disable Silent Mode
	{
		// End of Output Stream
    	printf("\r\n");
	}

    return err;
}
