/** @file

  Implementation of Function Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsFuncUtility.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include "ErrCode.h"
#include "InterfaceGet.h"
#include "ElanTsFuncApi.h"

/***************************************************
 * Function Implements
 ***************************************************/

int get_firmware_id(unsigned short *p_fw_id)
{
	int err = TP_SUCCESS;
    unsigned short fw_id = 0;

    err = send_fw_id_command();
    if(err != TP_SUCCESS)
        goto GET_FIRMWARE_ID_EXIT;

    err = get_fw_id_data(&fw_id);
    if(err != TP_SUCCESS)
        goto GET_FIRMWARE_ID_EXIT;

    *p_fw_id = fw_id;
    err = TP_SUCCESS;

GET_FIRMWARE_ID_EXIT:
	return err;
}

int get_fw_version(unsigned short *p_fw_version)
{
	int err = TP_SUCCESS;
    unsigned short fw_version = 0;

    err = send_fw_version_command();
    if(err != TP_SUCCESS)
        goto GET_FW_VERSION_EXIT;

    err = get_fw_version_data(&fw_version);
    if(err != TP_SUCCESS)
        goto GET_FW_VERSION_EXIT;

    *p_fw_version = fw_version;
    err = TP_SUCCESS;

GET_FW_VERSION_EXIT:
	return err;
}

int get_test_version(unsigned short *p_test_version)
{
	int err = TP_SUCCESS;
    unsigned short test_version = 0;

    err = send_test_version_command();
    if(err != TP_SUCCESS)
        goto GET_TEST_VERSION_EXIT;

    err = get_test_version_data(&test_version);
    if(err != TP_SUCCESS)
        goto GET_TEST_VERSION_EXIT;

    *p_test_version = test_version;
    err = TP_SUCCESS;

GET_TEST_VERSION_EXIT:
	return err;
}

int get_boot_code_version(unsigned short *p_bc_version)
{
	int err = TP_SUCCESS;
    unsigned short bc_version = 0;

    err = send_boot_code_version_command();
    if(err != TP_SUCCESS)
        goto GET_BOOT_CODE_VERSION_EXIT;

    err = get_boot_code_version_data(&bc_version);
    if(err != TP_SUCCESS)
        goto GET_BOOT_CODE_VERSION_EXIT;

    *p_bc_version = bc_version;
    err = TP_SUCCESS;

GET_BOOT_CODE_VERSION_EXIT:
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

int calibrate_touch_with_error_retry(int retry_count)
{
    int err = TP_SUCCESS,
        retry_index = 0;

	// Make Sure Retry Count Positive
	if(retry_count <= 0)
        retry_count = 1;

	for(retry_index = 0; retry_index < retry_count; retry_index++)
	{
		err = calibrate_touch();
		if(err == TP_SUCCESS)
		{
			// Without any error => Break retry loop and continue.
			break;
		}

		// With Error => Retry at most 3 times 
		DEBUG_PRINTF("%s: [%d/3] Fail to Calibrate Touch! errno=0x%x.\r\n", __func__, retry_index+1, err);
		if(retry_index == 2)
		{
			// Have retried for 3 times and can't fix it => Stop this function
			ERROR_PRINTF("%s: Fail to Get Information Page! errno=0x%x.\r\n", __func__, err); 
			goto CALIBRATE_TOUCH_WITH_ERROR_RETRY_EXIT;
		}
		else // retry_index = 0, 1
		{
			// wait 10ms
			usleep(10*1000); 
			continue;
		}		
	}

CALIBRATE_TOUCH_WITH_ERROR_RETRY_EXIT:
	return err;
}

// Hello Packet & BC Version
int get_hello_packet_bc_version(unsigned char *p_hello_packet, unsigned short *p_bc_version)
{
	int err = TP_SUCCESS;
	unsigned char hello_packet[4] = {0};
    unsigned short bc_version = 0;

	// Make Sure Page Data Buffer Valid
	if(p_hello_packet == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_HELLO_PACKET_BC_VERSION_EXIT;
	}

	// Send 7-bit I2C Slave Address
	err = send_request_hello_packet_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Send Request Hello Packet Command! errno=0x%x.\r\n", __func__, err);
		goto GET_HELLO_PACKET_BC_VERSION_EXIT;
	}

	// Receive Hello Packet
	err = read_data(hello_packet, sizeof(hello_packet), ELAN_READ_DATA_TIMEOUT_MSEC);
	if(err == TP_ERR_TIMEOUT)
	{
		DEBUG_PRINTF("%s: Fail to Receive Hello Packet! Timeout!\r\n", __func__);
		goto GET_HELLO_PACKET_BC_VERSION_EXIT;
	}
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Receive Hello Packet! errno=0x%x.\r\n", __func__, err);
		goto GET_HELLO_PACKET_BC_VERSION_EXIT;
	}
	DEBUG_PRINTF("vendor_cmd_data: %02x %02x %02x %02x.\r\n", hello_packet[0], hello_packet[1], hello_packet[2], hello_packet[3]);

	// Hello Packet
    DEBUG_PRINTF("Hello Packet: %02x.\r\n", hello_packet[0]);
	*p_hello_packet = hello_packet[0];

    // BC Version
    bc_version = (hello_packet[2] << 8) | hello_packet[3];
    DEBUG_PRINTF("BC Version: %02x.\r\n", bc_version);
    *p_bc_version = bc_version;

	// Success
	err = TP_SUCCESS;

GET_HELLO_PACKET_BC_VERSION_EXIT:
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

int get_and_update_info_page(unsigned char solution_id, unsigned char *info_page_buf, size_t info_page_buf_size)
{
	int err = TP_SUCCESS,
		page_data_index = 0;
	unsigned char info_page_data_buf[ELAN_FIRMWARE_PAGE_DATA_SIZE] = {0},
				  temp_info_page_buf[ELAN_FIRMWARE_PAGE_SIZE] = {0},
				  day		= 0,
				  month		= 0,
				  minute	= 0,
				  hour		= 0;
	unsigned short update_count				= 0,
				   year						= 0,
				   page_data				= 0,
				   page_checksum			= 0,
				   info_page_memory_address	= 0;
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

	// [Note] 2022/03/31
	// Support Info. Memory Space of Gen5 Series. 
	// Since we start to adopt Gen5 touch into chrome project, momory space of 32k flash should be taked into consideration.
	// Thus we have two type of information memory spaces:
	// Gen5 Touch (with 32k flash): info. memory space starts from 0x8040.
	// Gen6 Touch (with 64k flash): info. memory space starts from 0x0040.

	// Configure Page Address of Info. Page
	DEBUG_PRINTF("%s: solution_id=0x%02x.", __func__, solution_id);
	if (/* 63XX Solution */
		(solution_id == SOLUTION_ID_EKTH6315x1) || \
		(solution_id == SOLUTION_ID_EKTH6315x2) || \
		(solution_id == SOLUTION_ID_EKTH6315to5015M) || \
		(solution_id == SOLUTION_ID_EKTH6315to3915P) || \
		/* 73XX Solution */
		(solution_id == SOLUTION_ID_EKTH7315x1) || \
		(solution_id == SOLUTION_ID_EKTH7315x2) || \
		(solution_id == SOLUTION_ID_EKTH7318x1))
			info_page_memory_address = ELAN_INFO_PAGE_WRITE_MEMORY_ADDR;	// 63XX or 73XX: info_page_addr=0x0040
	else // 53XX Solution
			info_page_memory_address = ELAN_INFO_PAGE_MEMORY_ADDR;			// 53XX: info_page_addr=0x8040

	// Set Page Address
	temp_info_page_buf[0] = (unsigned char) (info_page_memory_address & 0x00FF);		// Low  Byte of Address
	temp_info_page_buf[1] = (unsigned char)((info_page_memory_address & 0xFF00) >> 8);	// High Byte of Address 
	
	// Set Page Data
	memcpy(&temp_info_page_buf[2], info_page_data_buf, ELAN_FIRMWARE_PAGE_DATA_SIZE);

	// Get Page Check Sum
	for(page_data_index = 0; page_data_index < (ELAN_FIRMWARE_PAGE_SIZE - 2); page_data_index += 2)
	{
		// Get Page Data
		page_data = (temp_info_page_buf[page_data_index + 1] << 8) | temp_info_page_buf[page_data_index];
		
		// If page address is 0x0040, replace it with 0x8040.
		if((page_data_index == 0) && (page_data == ELAN_INFO_PAGE_WRITE_MEMORY_ADDR)) // page_data[0]=0x0040
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

#if 0
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
#endif //0

int switch_to_boot_code(bool recovery)
{
	int err = TP_SUCCESS;

	// Enter IAP Mode
    if(recovery == false) // Normal IAP
	{
        // Write Flash Key
        err = send_write_flash_key_command();
        if(err != TP_SUCCESS)
	    {
		    ERROR_PRINTF("%s: [Normal IAP] Fail to Write Flash Key! errno=0x%x.\r\n", __func__, err);
		    goto SWITCH_TO_BOOT_CODE_EXIT;
	    }

        // Enter IAP Mode
	    err = send_enter_iap_command();
	    if(err != TP_SUCCESS)
	    {
		    ERROR_PRINTF("%s: [Normal IAP] Fail to Enter IAP Mode! errno=0x%x.\r\n", __func__, err);
		    goto SWITCH_TO_BOOT_CODE_EXIT;
	    }
    }
    else // Recovery IAP
    {
        /* [Note] 2020/02/10
		 * Do Not Send Enter IAP Command if Recovery Mode!
		 * Just Send Write Flash Key Command!
         * Migrate from V81.
		 */

        // Write Flash Key
        err = send_write_flash_key_command();
        if(err != TP_SUCCESS)
	    {
		    ERROR_PRINTF("%s: [Recovery IAP] Fail to Write Flash Key! errno=0x%x.\r\n", __func__, err);
		    goto SWITCH_TO_BOOT_CODE_EXIT;
	    }
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

// Page Data
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

