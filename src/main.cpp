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
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>       /* time_t, struct tm, time, localtime, asctime */
#include <sys/stat.h>
#include <sys/types.h>
#include "I2CHIDLinuxGet.h"
#include "ElanTsFuncUtility.h"

/***************************************************
 * Definitions
 ***************************************************/

// SW Version
#ifndef ELAN_TOOL_SW_VERSION
#define	ELAN_TOOL_SW_VERSION 	"2.9"
#endif //ELAN_TOOL_SW_VERSION

// File Length
#ifndef FILE_NAME_LENGTH_MAX
#define FILE_NAME_LENGTH_MAX	256
#endif //FILE_NAME_LENGTH_MAX

// Error Retry Count
#ifndef ERROR_RETRY_COUNT
#define ERROR_RETRY_COUNT	3
#endif //ERROR_RETRY_COUNT

// Result Log Information
#ifndef FILE_NAME_LENGTH_MAX
#define FILE_NAME_LENGTH_MAX	128
#endif //FILE_NAME_LENGTH_MAX

#ifdef __SUPPORT_RESULT_LOG__
#ifndef DEFAULT_LOG_FILENAME
#define DEFAULT_LOG_FILENAME	"/tmp/elan_i2chid_iap_result.txt"
#endif //DEFAULT_LOG_FILENAME
#endif //__SUPPORT_RESULT_LOG__

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

#ifdef __SUPPORT_RESULT_LOG__
// Result Log
char g_log_file[FILE_NAME_LENGTH_MAX] = {0};
#endif //__SUPPORT_RESULT_LOG__

// Silent Mode (Quiet)
bool g_quiet = false;

// Help Info.
bool g_help = false;

// Parameter Option Settings
#ifdef __SUPPORT_RESULT_LOG__
const char* const short_options = "p:P:f:oikl:qdh";
#else
const char* const short_options = "p:P:f:oikqdh";
#endif //__SUPPORT_RESULT_LOG__
const struct option long_options[] =
{
	{ "pid",					1, NULL, 'p'},
	{ "pid_hex",				1, NULL, 'P'},
	{ "file_path",				1, NULL, 'f'},
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

// Firmware File I/O
int open_firmware_file(char *filename, size_t filename_len, int *fd);
int close_firmware_file(int fd);
int get_firmware_size(int fd, int *firmware_size);
int compute_firmware_page_number(int firmware_size);
int retrieve_data_from_firmware(unsigned char *data, int data_size);

// Firmware Information
int get_firmware_information(bool quiet /* Silent Mode */);

// Calibration
int calibrate_touch(void);

// Hello Packet
int get_hello_packet(unsigned char *data);
int get_hello_packet_with_error_retry(unsigned char *data, int retry_count);

// Page/Frame Data
int read_page_data(unsigned short page_data_addr, unsigned short page_data_size, unsigned char *page_data_buf, size_t page_data_buf_size);
int write_frame_data(int data_offset, int data_len, unsigned char *frame_buf, int frame_buf_size);
int write_page_data(unsigned char *page_buf, int page_buf_size);

// Info. Page
int get_info_page(unsigned char *info_page_buf, size_t info_page_buf_size);
int get_info_page_with_error_retry(unsigned char *info_page_buf, size_t info_page_buf_size, int retry_count);
int get_and_update_info_page(unsigned char *info_page_buf, size_t info_page_buf_size);

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
 * File I/O
 ******************************************/

int open_firmware_file(char *filename, size_t filename_len, int *fd)
{
	int err = TP_SUCCESS,
		temp_fd = 0;

	// Make Sure Filename Valid
	if(filename == NULL)
	{
		ERROR_PRINTF("%s: NULL Filename String Pointer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto OPEN_FIRMWARE_FILE_EXIT;
	}	

	// Make Sure Filename Length Valid
	if(filename_len == 0)
	{
		ERROR_PRINTF("%s: Filename String Length is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto OPEN_FIRMWARE_FILE_EXIT;
	}

	DEBUG_PRINTF("Open file \"%s\".\r\n", filename);
	temp_fd = open(filename, O_RDONLY);
	if(temp_fd < 0)
	{
		ERROR_PRINTF("%s: Failed to open firmware file \'%s\', errno=0x%x.\r\n", __func__, filename, temp_fd);
		err = TP_ERR_FILE_NOT_FOUND;
		goto OPEN_FIRMWARE_FILE_EXIT;
	}

	DEBUG_PRINTF("File \"%s\" opened, fd=%d.\r\n", filename, temp_fd);
	*fd = temp_fd;

	// Success
	err = TP_SUCCESS;

OPEN_FIRMWARE_FILE_EXIT:
	return err;
}

int close_firmware_file(int fd)
{
	int err = TP_SUCCESS;
   
	if(fd >= 0)
	{
		err = close(fd);
		if(err < 0)
			ERROR_PRINTF("Failed to close firmware file(fd=%d), errno=0x%x.\r\n", fd, err);
	 	err = TP_ERR_IO_ERROR;
	}
      
	return err;
}

int get_firmware_size(int fd, int *firmware_size)
{
	int err = TP_SUCCESS;
	struct stat file_stat;
   
	err = fstat(fd, &file_stat);
	if(err < 0)
	{
		ERROR_PRINTF("Fail to get firmware file size, errno=0x%x.\r\n", err);
		err = TP_ERR_FILE_NOT_FOUND;
	}
	else
	{
		DEBUG_PRINTF("File size=%zd.\r\n", file_stat.st_size);
		*firmware_size = file_stat.st_size;
		err = TP_SUCCESS;
	}
   
	return err;
}

int compute_firmware_page_number(int firmware_size)
{
   /*******************************
   * [ELAN Firmware Page]
   * Address:    1 Word (= 2 Byte)
   * Page Data: 64 Word
   * Checksum:   1 Word
   * ------------------------------ 
   * Total: 1 + 64 + 1 = 66 Word 
   *                   = 132 Byte
   *******************************/
   return ((firmware_size / ELAN_FIRMWARE_PAGE_SIZE) + ((firmware_size % ELAN_FIRMWARE_PAGE_SIZE) != 0));
}

int retrieve_data_from_firmware(unsigned char *data, int data_size)
{
	int err = TP_SUCCESS;

	// Make Sure Page Data Buffer Valid
	if(data == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto RETRIEVE_DATA_FROM_FIRMWARE_EXIT;
	}	

	// Make Sure Page Data Buffer Size Valid
	if(data_size == 0)
	{
		ERROR_PRINTF("%s: Data Buffer Size is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto RETRIEVE_DATA_FROM_FIRMWARE_EXIT;
	}

	// Read Data from File   
	err = read(g_firmware_fd, data, data_size);
	if(err != data_size)
	{
		ERROR_PRINTF("%s: Fail to get %d bytes from fd %d! (result=%d, errno=%d)\r\n", __func__, data_size, g_firmware_fd, err, errno);
		err = TP_GET_DATA_FAIL;
	}

	// Success
	err = TP_SUCCESS;

RETRIEVE_DATA_FROM_FIRMWARE_EXIT:     
	return err;
}

/*******************************************
 * Function Implementation
 ******************************************/

int get_firmware_information(bool quiet /* Silent Mode */)
{
	int err = TP_SUCCESS;

	if(quiet == true) // Enable Silent Mode
	{
		// Firmware Version
    	err = send_fw_version_command();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		err = read_fw_version_data(true /* Enable Silent Mode */);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
	}
	else // Disable Silent Mode
	{
		printf("--------------------------------\r\n");

		// FW ID
		err = send_fw_id_command();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		err = read_fw_id_data();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		// Firmware Version
    	err = send_fw_version_command();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		//err = read_fw_version_data();
		err = read_fw_version_data(false /* Disable Silent Mode */);
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		// Test Version
    	err = send_test_version_command();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

    	err = read_test_version_data();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		// Boot Code Version
    	err = send_boot_code_version_command();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;

		err = read_boot_code_version_data();
		if(err != TP_SUCCESS)
			goto GET_FW_INFO_EXIT;
	}

GET_FW_INFO_EXIT:
	return err;
}

int calibrate_touch(void)
{
	int err = TP_SUCCESS;

	printf("--------------------------------\r\n");

	// Send Command
	err = send_rek_command();
	if(err != TP_SUCCESS)
		goto CALIBRATE_TOUCH_EXIT;

	// Receive Data
	err = receive_rek_response();
	if(err != TP_SUCCESS)
		goto CALIBRATE_TOUCH_EXIT;

CALIBRATE_TOUCH_EXIT:
	return err;
}

// Hello Packet
int get_hello_packet(unsigned char *data)
{
	int err = TP_SUCCESS;
	unsigned char hello_packet[4] = {0};

	// Make Sure Page Data Buffer Valid
	if(data == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_HELLO_PACKET_EXIT;
	}

	// Send 7-bit I2C Slave Address
	err = send_request_hello_packet_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Send Request Hello Packet Command! errno=0x%x.\r\n", __func__, err);
		goto GET_HELLO_PACKET_EXIT;
	}

	// Receive Hello Packet
	err = read_data(hello_packet, sizeof(hello_packet), ELAN_READ_DATA_TIMEOUT_MSEC);
	if(err == TP_ERR_TIMEOUT)
	{
		DEBUG_PRINTF("%s: Fail to Receive Hello Packet! Timeout!\r\n", __func__);
		goto GET_HELLO_PACKET_EXIT;
	}
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Receive Hello Packet! errno=0x%x.\r\n", __func__, err);
		goto GET_HELLO_PACKET_EXIT;
	}
	DEBUG_PRINTF("vendor_cmd_data: %02x %02x %02x %02x.\r\n", hello_packet[0], hello_packet[1], hello_packet[2], hello_packet[3]);

	// Copy first byte of Hello Packet to Input Buffer
	*data = hello_packet[0];

	// Success
	err = TP_SUCCESS;

GET_HELLO_PACKET_EXIT:
	return err;
}

int get_hello_packet_with_error_retry(unsigned char *data, int retry_count)
{
	int err = TP_SUCCESS,
		retry_index = 0;

	// Make Sure Retry Count Positive
	if(retry_count <= 0)
		retry_count = 1;

	for(retry_index = 0; retry_index < retry_count; retry_index++)
	{
		err = get_hello_packet(data);
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

int read_page_data(unsigned short page_data_addr, unsigned short page_data_size, unsigned char *page_data_buf, size_t page_data_buf_size)
{
	int err = TP_SUCCESS;
	unsigned int page_frame_index = 0,
				 page_frame_count = 0,
				 page_frame_data_len = 0,
				 data_len = 0,
				 page_data_index = 0;
	unsigned char temp_page_data_buf[ELAN_FIRMWARE_PAGE_DATA_SIZE] = {0},
				  data_buf[ELAN_I2CHID_DATA_BUFFER_SIZE] = {0};

	// Make Sure Page Data Buffer Valid
	if(page_data_buf == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto READ_PAGE_DATA_EXIT;
	}	

	// Make Sure Page Data Buffer Size Valid
	if(page_data_buf_size == 0)
	{
		ERROR_PRINTF("%s: Page Data Buffer Size is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto READ_PAGE_DATA_EXIT;
	}

	// Send Show Bulk ROM Data Command
	err = send_show_bulk_rom_data_command(page_data_addr, (page_data_size / 2) /* unit: word */);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Send Show Bulk ROM Data Command! errno=0x%x.\r\n", __func__, err);
		goto READ_PAGE_DATA_EXIT;
	}

	// wait 20ms
	usleep(20*1000); 

	// Receive Page Data
	page_frame_count = (page_data_size / ELAN_I2CHID_READ_PAGE_FRAME_SIZE) + \
					  ((page_data_size % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) != 0);
	for(page_frame_index = 0; page_frame_index < page_frame_count; page_frame_index++)
	{
		// Clear Data Buffer
		memset(data_buf, 0, sizeof(data_buf));

		// Data Length
		if((page_frame_index == (page_frame_count - 1)) && ((page_data_size % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) != 0)) // Last Frame
			page_frame_data_len = page_data_size % ELAN_I2CHID_READ_PAGE_FRAME_SIZE;
		else // (page_frame_index != (page_frame_count -1)) || ((ELAN_FIRMWARE_PAGE_DATA_SIZE % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) == 0)
			page_frame_data_len = ELAN_I2CHID_READ_PAGE_FRAME_SIZE;
		data_len = 3 /* 1(Packet Header 0x99) + 1(Packet Index) + 1(Data Length) */ + page_frame_data_len;

		// Read $(page_frame_index)-th Bulk Page Data to Buffer
		err = read_data(data_buf, data_len, ELAN_READ_DATA_TIMEOUT_MSEC);
		if(err != TP_SUCCESS) // Error or Timeout
		{
			ERROR_PRINTF("%s: [%d] Fail to Read %d-Byte Data! errno=0x%x.\r\n", __func__, page_frame_index, data_len, err);
			goto READ_PAGE_DATA_EXIT;
		}

		// Copy Read Data to Page Buffer
		memcpy(&temp_page_data_buf[page_data_index], &data_buf[3], page_frame_data_len);
		page_data_index += page_frame_data_len;
	}

	// Copy Page Data to Input Buffer
	memcpy(page_data_buf, temp_page_data_buf, page_data_size);

	// Success
	err = TP_SUCCESS;

READ_PAGE_DATA_EXIT:
	return err;
}

// Info. Page
int get_info_page(unsigned char *info_page_buf, size_t info_page_buf_size)
{
	int err = TP_SUCCESS;

	// Make Sure Info. Page Buffer Valid
	if(info_page_buf == NULL)
	{
		ERROR_PRINTF("%s: NULL Info. Page Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_INFO_PAGE_EXIT;
	}	

	// Make Sure Info. Page Buffer Size Valid
	if(info_page_buf_size == 0)
	{
		ERROR_PRINTF("%s: Info. Page Buffer Size is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_INFO_PAGE_EXIT;
	}

	// Enter Test Mode
	err = send_enter_test_mode_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Enter Test Mode! errno=0x%x.\r\n", __func__, err);
		goto GET_INFO_PAGE_EXIT;
	}

	// Read Information Page
	err = read_page_data(ELAN_INFO_PAGE_MEMORY_ADDR, ELAN_FIRMWARE_PAGE_DATA_SIZE, info_page_buf, info_page_buf_size);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Get Information Page! errno=0x%x.\r\n", __func__, err);
		goto GET_INFO_PAGE_EXIT_1;
	}

	// Leave Test Mode
	err = send_exit_test_mode_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Leave Test Mode! errno=0x%x.\r\n", __func__, err);
		goto GET_INFO_PAGE_EXIT;
	}

	// Success
	err = TP_SUCCESS;

GET_INFO_PAGE_EXIT:
	return err;

GET_INFO_PAGE_EXIT_1:
	// Leave Test Mode
	send_exit_test_mode_command();

	return err;
}

int get_info_page_with_error_retry(unsigned char *info_page_buf, size_t info_page_buf_size, int retry_count)
{
	int err = TP_SUCCESS,
		retry_index = 0;

	// Make Sure Retry Count Positive
	if(retry_count <= 0)
		retry_count = 1;

	for(retry_index = 0; retry_index < retry_count; retry_index++)
	{
		err = get_info_page(info_page_buf, info_page_buf_size);
		if(err == TP_SUCCESS)
		{
			// Without any error => Break retry loop and continue.
			break;
		}

		// With Error => Retry at most 3 times 
		DEBUG_PRINTF("%s: [%d/3] Fail to Get Information Page! errno=0x%x.\r\n", __func__, retry_index+1, err);
		if(retry_index == 2)
		{
			// Have retried for 3 times and can't fix it => Stop this function
			ERROR_PRINTF("%s: Fail to Get Information Page! errno=0x%x.\r\n", __func__, err); 
			goto GET_INFO_PAGE_WITH_ERROR_RETRY_EXIT;
		}
		else // retry_index = 0, 1
		{
			// wait 50ms
			usleep(50*1000); 

			continue;
		}		
	}

GET_INFO_PAGE_WITH_ERROR_RETRY_EXIT:
	return err;
}

int get_and_update_info_page(unsigned char *info_page_buf, size_t info_page_buf_size)
{
	int err = TP_SUCCESS,
		page_data_index = 0;
	unsigned char info_page_data_buf[ELAN_FIRMWARE_PAGE_DATA_SIZE] = {0},
				  temp_info_page_buf[ELAN_FIRMWARE_PAGE_SIZE] = {0},
				  day		= 0,
				  month		= 0,
				  minute	= 0,
				  hour		= 0;
	unsigned short update_count		= 0,
				   year				= 0,
				   page_data		= 0,
				   page_checksum	= 0;
	time_t cur_time;
	struct tm *time_info;

	// Make Sure Info. Page Buffer Valid
	if(info_page_buf == NULL)
	{
		ERROR_PRINTF("%s: NULL Info. Page Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_AND_UPDATE_INFO_PAGE_EXIT;
	}	

	// Make Sure Info. Page Buffer Size Valid
	if(info_page_buf_size == 0)
	{
		ERROR_PRINTF("%s: Info. Page Buffer Size is Zero!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_AND_UPDATE_INFO_PAGE_EXIT;
	}
	
	/* Get Inforamtion Page */
	err = get_info_page_with_error_retry(info_page_data_buf, sizeof(info_page_data_buf), ERROR_RETRY_COUNT);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Get Information Page! errno=0x%x.\r\n", __func__, err);
		goto GET_AND_UPDATE_INFO_PAGE_EXIT;
	}

	//
	// Get Counter & Time Info. from Infomation Page
	//
	// [Note] 2019/04/12
	// Ordering with Little-Endian in Reading
	//
	update_count	= (info_page_data_buf[65] << 8) | info_page_data_buf[64]; // Addr: 0x8060 (in word) // 29 02
	year			= (info_page_data_buf[67] << 8) | info_page_data_buf[66]; // Addr: 0x8061 (in word) // 17 20
	day				=  info_page_data_buf[68]; // Addr: 0x8062 (Low  Byte) // 03
	month			=  info_page_data_buf[69]; // Addr: 0x8062 (High Byte) // 11
	minute			=  info_page_data_buf[70]; // Addr: 0x8063 (Low  Byte) // 30
	hour			=  info_page_data_buf[71]; // Addr: 0x8063 (High Byte) // 
	DEBUG_PRINTF("%s: Previous count=%d, %04d/%02d/%02d %02d:%02d.\r\n", __func__, update_count, year, month, day, hour, minute);

	// Get Current Time
	time(&cur_time);
	time_info = localtime(&cur_time);
	//DEBUG_PRINTF("%s: Current date & time: %s.\r\n", __func__, asctime(time_info));

	update_count++;
	DEBUG_PRINTF("%s: Current count=%d, %04d/%02d/%02d %02d:%02d.\r\n", __func__, update_count, \
																1900 + time_info->tm_year, \
																1 + time_info->tm_mon, \
																time_info->tm_mday, \
																time_info->tm_hour, \
																time_info->tm_min);

	//
	// Set Info. Page Data
	//
	// [Note] 2019/04/12
	// Ordering with Big-Endian in Writing
	//
	info_page_data_buf[64] = (unsigned char)((update_count & 0xFF00) >> 8);					// High Byte of Count
	info_page_data_buf[65] = (unsigned char) (update_count & 0x00FF);						// Low  Byte of Count 
	info_page_data_buf[66] = (unsigned char)(((1900 + time_info->tm_year) & 0xFF00) >> 8);	// High Byte of Year
	info_page_data_buf[67] = (unsigned char) ((1900 + time_info->tm_year) & 0x00FF);		// Low  Byte of Year
	info_page_data_buf[68] = (unsigned char)(1 + time_info->tm_mon); 						// Month
	info_page_data_buf[69] = (unsigned char)time_info->tm_mday; 							// Day
	info_page_data_buf[70] = (unsigned char)time_info->tm_hour; 							// Hour
	info_page_data_buf[71] = (unsigned char)time_info->tm_min; 								// Minute

	/* Update Inforamtion Page */

	// Set Page Address
	temp_info_page_buf[0] = (unsigned char) (ELAN_INFO_PAGE_WRITE_MEMORY_ADDR & 0x00FF);			// Low  Byte of Address
	temp_info_page_buf[1] = (unsigned char)((ELAN_INFO_PAGE_WRITE_MEMORY_ADDR & 0xFF00) >> 8);	// High Byte of Address 
	
	// Set Page Data
	memcpy(&temp_info_page_buf[2], info_page_data_buf, ELAN_FIRMWARE_PAGE_DATA_SIZE);

	// Get Page Check Sum
	for(page_data_index = 0; page_data_index < (ELAN_FIRMWARE_PAGE_SIZE - 2); page_data_index += 2)
	{
		// Get Page Data
		page_data = (temp_info_page_buf[page_data_index + 1] << 8) | temp_info_page_buf[page_data_index];
		
		// If page address is 0x0040, replace it with 0x8040.
		if((page_data_index == 0) && (page_data == ELAN_INFO_PAGE_WRITE_MEMORY_ADDR))
			page_data = ELAN_INFO_PAGE_MEMORY_ADDR;
		
		// Update Checksum
		page_checksum += page_data;
	}
	DEBUG_PRINTF("%s: Checksum=0x%04x.\r\n", __func__, page_checksum);

	// Set Page CheckSum
	temp_info_page_buf[ELAN_FIRMWARE_PAGE_SIZE - 2] = (unsigned char) (page_checksum & 0x00FF);			// Low  Byte of Checksum
	temp_info_page_buf[ELAN_FIRMWARE_PAGE_SIZE - 1] = (unsigned char)((page_checksum & 0xFF00) >> 8);	// High Byte of Checksum 

	// Copy Info. Page Data to Input Buffer
	memcpy(info_page_buf, temp_info_page_buf, info_page_buf_size);

	// Success
	err = TP_SUCCESS;

GET_AND_UPDATE_INFO_PAGE_EXIT:
	return err;
}

int check_slave_address(void)
{
	int err = TP_SUCCESS;
	unsigned char slave_addr = 0;

	// Send 7-bit I2C Slave Address
	err = send_slave_address();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Send Slave Address! errno=0x%x.\r\n", __func__, err);
		goto CHECK_SLAVE_ADDRESS_EXIT;
	}

	// Read Slave Address
	err = read_data(&slave_addr, 1, ELAN_READ_DATA_TIMEOUT_MSEC);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Read Slave Address! errno=0x%x.\r\n", __func__, err);
		goto CHECK_SLAVE_ADDRESS_EXIT;
	}

	// Check if 8-bit I2C Slave Address
	if(slave_addr != ELAN_I2C_SLAVE_ADDR)
	{
		ERROR_PRINTF("%s: Not Elan Touch Device! Address=0x%x.\r\n", __func__, slave_addr);
		err = TP_ERR_DATA_PATTERN;
		goto CHECK_SLAVE_ADDRESS_EXIT;
	}

	// Success
	err = TP_SUCCESS;

CHECK_SLAVE_ADDRESS_EXIT:
	return err;
}

int switch_to_boot_code(void)
{
	int err = TP_SUCCESS;

	// Enter IAP Mode
	err = send_enter_iap_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Enter IAP Mode! errno=0x%x.\r\n", __func__, err);
		goto SWITCH_TO_BOOT_CODE_EXIT;
	}

	// wait 15ms
	usleep(15*1000); 

	// Check Slave Address
	err = check_slave_address();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Check Slave Address! errno=0x%x.\r\n", __func__, err);
		goto SWITCH_TO_BOOT_CODE_EXIT;
	}

	// Success
	err = TP_SUCCESS;

SWITCH_TO_BOOT_CODE_EXIT:
	return err;
}

// Write Firmware Page
int write_frame_data(int data_offset, int data_len, unsigned char *frame_buf, int frame_buf_size)
{
	int err = TP_SUCCESS;
	unsigned char hid_frame_data[ELAN_I2CHID_OUTPUT_BUFFER_SIZE] = {0};

	// Validate Data Length
	if((data_len == 0) || (data_len > ELAN_I2CHID_PAGE_FRAME_SIZE))
	{
		ERROR_PRINTF("%s: Invalid Data Length: %d.\r\n", __func__, data_len);
		err = TP_ERR_INVALID_PARAM;
		goto WRITE_FRAME_DATA_EXIT;
	}

	// Valid Frame Buffer
	if(frame_buf == NULL)
	{
		ERROR_PRINTF("%s: NULL Frame Buffer Pointer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto WRITE_FRAME_DATA_EXIT;
	}

	// Valid Frame Buffer Size
	if((frame_buf_size < data_len) || (frame_buf_size > ELAN_I2CHID_OUTPUT_BUFFER_SIZE))
	{
		ERROR_PRINTF("%s: Invalid Frame Buffer Size: %d.\r\n", __func__, frame_buf_size);
		err = TP_ERR_INVALID_PARAM;
		goto WRITE_FRAME_DATA_EXIT;
	}

	// Add header of vendor command to frame data
	hid_frame_data[0] = ELAN_HID_OUTPUT_REPORT_ID;
	hid_frame_data[1] = 0x21;
	hid_frame_data[2] = (unsigned char)((data_offset & 0xFF00) >> 8);	// High Byte of Data Offset //ex:00
	hid_frame_data[3] = (unsigned char) (data_offset & 0x00FF);			// Low  Byte of Data Offset //ex:1B
	hid_frame_data[4] = data_len;
	memcpy(&hid_frame_data[5], frame_buf, frame_buf_size);
   
	// Write frame data to touch
	err = __hidraw_write(hid_frame_data, sizeof(hid_frame_data), ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if(err != TP_SUCCESS)
		ERROR_PRINTF("Fail to write frame data, errno=%d.\r\n", err);

WRITE_FRAME_DATA_EXIT:
	return err;
}

int write_page_data(unsigned char *page_buf, int page_buf_size)
{
    int err = TP_SUCCESS,
        frame_index = 0,
        frame_count = 0,
		frame_data_len = 0,
		start_index = 0;
	unsigned char temp_page_buf[ELAN_FIRMWARE_PAGE_SIZE * 30] = {0};

	// Valid Page Buffer
	if(page_buf == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Buffer Pointer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto WRITE_PAGE_DATA_EXIT;
	}

	// Validate Page Buffer Size
	if((page_buf_size == 0) || (page_buf_size > (ELAN_FIRMWARE_PAGE_SIZE * 30)))
	{
		ERROR_PRINTF("%s: Invalid Page Buffer Size: %d.\r\n", __func__, page_buf_size);
		err = TP_ERR_INVALID_PARAM;
		goto WRITE_PAGE_DATA_EXIT;
	}

	// Copy Page Data from Input Page Buffer to Temp Page Buffer
	memcpy(temp_page_buf, page_buf, page_buf_size);
                  
	// Get Frame Count
	frame_count = (page_buf_size / ELAN_I2CHID_PAGE_FRAME_SIZE) + 
				 ((page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE) != 0);
    
    // Write Page Data with Frames
    for(frame_index = 0; frame_index < frame_count; frame_index++)
    {
		if((frame_index == (frame_count - 1)) && ((page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE) > 0)) // The Last Frame
			frame_data_len = page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE;
		else
			frame_data_len = ELAN_I2CHID_PAGE_FRAME_SIZE;
       
		// Write Frame Data
		err = write_frame_data(start_index, frame_data_len, &temp_page_buf[start_index], frame_data_len);
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to Update Page from 0x%x (Length %d)! errno=%d.\r\n", __func__, start_index, frame_data_len, err);
			goto WRITE_PAGE_DATA_EXIT;
		}
        
		// Update Start Index to Next Frame
		start_index += frame_data_len;
    }

	// Request Flash Write
	err = send_flash_write_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Request Flash Write! errno=%d.\r\n", __func__, err);
		goto WRITE_PAGE_DATA_EXIT;
	}

	// Wait for FW Writing Flash
	if(page_buf_size == (ELAN_FIRMWARE_PAGE_SIZE * 30)) // 30 Page Block
		usleep(360 * 1000); // wait 12ms * 30
	else
		usleep(15 * 1000); // wait 15ms

	// Receive Response of Flash Write
	err = receive_flash_write_response();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Receive Flash Write! errno=%d.\r\n", __func__, err);
		goto WRITE_PAGE_DATA_EXIT;
	}

    // Success
	err = TP_SUCCESS;
                                   
WRITE_PAGE_DATA_EXIT:                  
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
				  page_block_buf[ELAN_FIRMWARE_PAGE_SIZE * 30] = {0};
    bool bDisableOutputBufferDebug = false;

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

	if(recovery == false) // Normal Mode
	{
		//
		// Get & Update Information Page
		//
		err = get_and_update_info_page(info_page_buf, sizeof(info_page_buf));
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to get/update Inforamtion Page! errno=0x%x.\r\n", __func__, err);
			goto UPDATE_FIRMWARE_EXIT;
		}
	}
	
	printf("Start FW Update Process...\r\n");

	//
	// Switch to Boot Code
	//
	err = switch_to_boot_code();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to switch to Boot Code! errno=0x%x.\r\n", __func__, err);
		goto UPDATE_FIRMWARE_EXIT;
	}

	// 
	// Update with FW Pages 
	//

    if((g_bEnableOutputBufferDebug == true) && (g_debug == false))
    {
        // Disable Output Buffer Debug
        DEBUG_PRINTF("Disable Output Buffer Debug.\r\n");
        g_bEnableOutputBufferDebug = false;
        bDisableOutputBufferDebug = true;
    }

	if(recovery == false) // Normal Mode
	{
		// Write Information Page
		DEBUG_PRINTF("Update Infomation Page...\r\n");
		err = write_page_data(info_page_buf, sizeof(info_page_buf));
		if(err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to Write Infomation Page! errno=0x%x.\r\n", __func__, err);
			goto UPDATE_FIRMWARE_EXIT;
		}
	}
	
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
		if((block_index == (block_count - 1)) && ((block_count % 30) != 0)) // Last Block
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
    
    if(bDisableOutputBufferDebug == true)
    {
        // Re-Enable Output Buffer Debug
        DEBUG_PRINTF("Re-Enable Output Buffer Debug.\r\n");
        g_bEnableOutputBufferDebug = true;
    }

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
		file_path_len = 0;
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
                ERROR_PRINTF("%s: Unknow Command!\r\n", __func__);
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
        goto EXIT;

	if(g_quiet == false) // Disable Silent Mode
		printf("i2chid_iap v%s.\r\n", ELAN_TOOL_SW_VERSION);

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
		err = calibrate_touch() ;
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
		err = calibrate_touch() ;
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
