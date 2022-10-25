/** @file

  Implementation of Function APIs for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsFuncApi.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include <time.h>       		/* time_t, struct tm, time, localtime, asctime */
#include "InterfaceGet.h"
#include "ElanTsI2chidUtility.h"
#include "ElanTsFwFileIoUtility.h"
#include "ElanTsFuncApi.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

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

// Solution ID
int get_solution_id(unsigned char *p_solution_id)
{
    int err = TP_SUCCESS;
    unsigned short fw_version = 0;
    unsigned char  solution_id = 0;

    // Check if Parameter Invalid
    if (p_solution_id == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_solution_id=0x%p)\r\n", __func__, p_solution_id);
        err = TP_ERR_INVALID_PARAM;
        goto GET_SOLUTION_ID_EXIT;
    }

    // Get Firmware Version
    err = get_fw_version(&fw_version);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get FW Version! err=0x%x.\r\n", __func__, err);
        goto GET_SOLUTION_ID_EXIT;
    }
    DEBUG_PRINTF("%s: FW Version: 0x%04x.\r\n", __func__, fw_version);

    // Get Solution ID
    solution_id = HIGH_BYTE(fw_version);
    DEBUG_PRINTF("Solution ID: 0x%02x.\r\n", solution_id);

    *p_solution_id = solution_id;
    err = TP_SUCCESS;

GET_SOLUTION_ID_EXIT:
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
        DEBUG_PRINTF("%s: [%d/3] Fail to Calibrate Touch! err=0x%x.\r\n", __func__, retry_index+1, err);
        if(retry_index == 2)
        {
            // Have retried for 3 times and can't fix it => Stop this function
            ERROR_PRINTF("%s: Fail to Get Information Page! err=0x%x.\r\n", __func__, err);
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

int get_rek_counter(unsigned short *p_rek_counter)
{
    int err = TP_SUCCESS;
    unsigned short rek_counter = 0;

    // Validate Input Buffer
    if(p_rek_counter == NULL)
    {
        ERROR_PRINTF("%s: NULL ReK Counter Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GET_REK_COUNTER_EXIT;
    }

    // Send Command
    err = send_rek_counter_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send ReK Counter Command! err=0x%x.\r\n", __func__, err);
        goto GET_REK_COUNTER_EXIT;
    }

    // Receive Data
    err = receive_rek_counter_data(&rek_counter);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive ReK Counter Data! err=0x%x.\r\n", __func__, err);
        goto GET_REK_COUNTER_EXIT;
    }

    // Load ReK Counter to Input Buffer
    *p_rek_counter = rek_counter;

    // Success
    err = TP_SUCCESS;

GET_REK_COUNTER_EXIT:
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
        ERROR_PRINTF("%s: Fail to Send Request Hello Packet Command! err=0x%x.\r\n", __func__, err);
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
        ERROR_PRINTF("%s: Fail to Receive Hello Packet! err=0x%x.\r\n", __func__, err);
        goto GET_HELLO_PACKET_BC_VERSION_EXIT;
    }
    DEBUG_PRINTF("vendor_cmd_data: %02x %02x %02x %02x.\r\n", hello_packet[0], hello_packet[1], hello_packet[2], hello_packet[3]);

    // Hello Packet
    DEBUG_PRINTF("Hello Packet: %02x.\r\n", hello_packet[0]);
    *p_hello_packet = hello_packet[0];

    // BC Version
    bc_version = (hello_packet[2] << 8) | hello_packet[3];
    DEBUG_PRINTF("BC Version: %04x.\r\n", bc_version);
    *p_bc_version = bc_version;

    // Success
    err = TP_SUCCESS;

GET_HELLO_PACKET_BC_VERSION_EXIT:
    return err;
}

int get_hello_packet_bc_version_with_error_retry(unsigned char *p_hello_packet, unsigned short *p_bc_version, int retry_count)
{
    int err = TP_SUCCESS,
        retry_index = 0;

    // Make Sure Page Data Buffer Valid
    if(p_hello_packet == NULL)
    {
        ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GET_HELLO_PACKET_BC_VERSION_WITH_ERROR_RETRY_EXIT;
    }

    // Make Sure Retry Count Positive
    if(retry_count <= 0)
        retry_count = 1;

    for(retry_index = 0; retry_index < retry_count; retry_index++)
    {
        err = get_hello_packet_bc_version(p_hello_packet, p_bc_version);
        if(err == TP_SUCCESS)
        {
            // Without any error => Break retry loop and continue.
            break;
        }

        // With Error => Retry at most 3 times
        DEBUG_PRINTF("%s: [%d/3] Fail to Get Hello Packet (& BC Version)! err=0x%x.\r\n", __func__, retry_index+1, err);
        if(retry_index == 2)
        {
            // Have retried for 3 times and can't fix it => Stop this function
            ERROR_PRINTF("%s: Fail to Get Hello Packet (& BC Version)! err=0x%x.\r\n", __func__, err);
            goto GET_HELLO_PACKET_BC_VERSION_WITH_ERROR_RETRY_EXIT;
        }
        else // retry_index = 0, 1
        {
            // wait 50ms
            usleep(50*1000);

            continue;
        }
    }

GET_HELLO_PACKET_BC_VERSION_WITH_ERROR_RETRY_EXIT:
    return err;
}

int get_hello_packet_with_error_retry(unsigned char *p_hello_packet, int retry_count)
{
    int err = TP_SUCCESS,
        retry_index = 0;
    unsigned short bc_bc_version = 0;

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
        err = get_hello_packet_bc_version(p_hello_packet, &bc_bc_version);
        if(err == TP_SUCCESS)
        {
            // Without any error => Break retry loop and continue.
            break;
        }

        // With Error => Retry at most 3 times
        DEBUG_PRINTF("%s: [%d/3] Fail to Get Hello Packet! err=0x%x.\r\n", __func__, retry_index+1, err);
        if(retry_index == 2)
        {
            // Have retried for 3 times and can't fix it => Stop this function
            ERROR_PRINTF("%s: Fail to Get Hello Packet! err=0x%x.\r\n", __func__, err);
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

int read_memory_page(unsigned short mem_page_address, unsigned short mem_page_size, unsigned char *p_mem_page_buf, size_t mem_page_buf_size)
{
    int err = TP_SUCCESS;
    unsigned int page_frame_index = 0,
                 page_frame_count = 0,
                 page_frame_data_len = 0,
                 data_len = 0,
                 page_data_index = 0;
    unsigned char mem_page_buf[ELAN_MEMORY_PAGE_SIZE] = {0},
                  data_buf[ELAN_I2CHID_DATA_BUFFER_SIZE] = {0};

    // Make Sure Page Data Buffer Valid
    if(p_mem_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto READ_MEMORY_PAGE_EXIT;
    }

    // Make Sure Page Data Buffer Size Valid
    if(mem_page_buf_size < ELAN_MEMORY_PAGE_SIZE)
    {
        ERROR_PRINTF("%s: Invalid Memory Page Buffer Size (%ld)!\r\n", __func__, mem_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto READ_MEMORY_PAGE_EXIT;
    }

    // Send Show Bulk ROM Data Command
    err = send_show_bulk_rom_data_command(mem_page_address, (mem_page_size / 2) /* unit: word */);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Show Bulk ROM Data Command! err=0x%x.\r\n", __func__, err);
        goto READ_MEMORY_PAGE_EXIT;
    }

    // wait 20ms
    usleep(20*1000);

    // Receive Page Data
    page_frame_count = (mem_page_size / ELAN_I2CHID_READ_PAGE_FRAME_SIZE) + \
                       ((mem_page_size % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) != 0);
    for(page_frame_index = 0; page_frame_index < page_frame_count; page_frame_index++)
    {
        // Clear Data Buffer
        memset(data_buf, 0, sizeof(data_buf));

        // Data Length
        if((page_frame_index == (page_frame_count - 1)) && ((mem_page_size % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) != 0)) // Last Frame
            page_frame_data_len = mem_page_size % ELAN_I2CHID_READ_PAGE_FRAME_SIZE;
        else // (page_frame_index != (page_frame_count -1)) || ((ELAN_MEMORY_PAGE_SIZE % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) == 0)
            page_frame_data_len = ELAN_I2CHID_READ_PAGE_FRAME_SIZE;
        data_len = 3 /* 1(Packet Header 0x99) + 1(Packet Index) + 1(Data Length) */ + page_frame_data_len;

        // Read $(page_frame_index)-th Bulk Page Data to Buffer
        err = read_data(data_buf, data_len, ELAN_READ_DATA_TIMEOUT_MSEC);
        if(err != TP_SUCCESS) // Error or Timeout
        {
            ERROR_PRINTF("%s: [%d] Fail to Read %d-Byte Data! err=0x%x.\r\n", __func__, page_frame_index, data_len, err);
            goto READ_MEMORY_PAGE_EXIT;
        }

        // Copy Read Data to Page Buffer
        memcpy(&mem_page_buf[page_data_index], &data_buf[3], page_frame_data_len);
        page_data_index += page_frame_data_len;
    }

    // Copy Page Data to Input Buffer
    memcpy(p_mem_page_buf, mem_page_buf, sizeof(mem_page_buf));

    // Success
    err = TP_SUCCESS;

READ_MEMORY_PAGE_EXIT:
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
        ERROR_PRINTF("%s: Fail to Enter Test Mode! err=0x%x.\r\n", __func__, err);
        goto GET_INFO_PAGE_EXIT;
    }

    // Read Information Page
    err = read_memory_page(ELAN_INFO_MEMORY_PAGE_1_ADDR, ELAN_MEMORY_PAGE_SIZE, info_page_buf, info_page_buf_size);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Information Page! err=0x%x.\r\n", __func__, err);
        goto GET_INFO_PAGE_EXIT_1;
    }

    // Leave Test Mode
    err = send_exit_test_mode_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Leave Test Mode! err=0x%x.\r\n", __func__, err);
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
        DEBUG_PRINTF("%s: [%d/3] Fail to Get Information Page! err=0x%x.\r\n", __func__, retry_index+1, err);
        if(retry_index == 2)
        {
            // Have retried for 3 times and can't fix it => Stop this function
            ERROR_PRINTF("%s: Fail to Get Information Page! err=0x%x.\r\n", __func__, err);
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

int set_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short value)
{
    int err = TP_SUCCESS;
    unsigned short data_index = 0; // Memory Address Offset

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto SET_INFO_PAGE_VALUE_EXIT;
    }

    // Address
    if(address < ELAN_INFO_MEMORY_PAGE_1_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%04x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto SET_INFO_PAGE_VALUE_EXIT;
    }

    // Data Index
    data_index = (address - ELAN_INFO_MEMORY_PAGE_1_ADDR) * 2 /* unit: byte */;

    //
    // [Note] 2022/08/03
    // Ordering with little-endian, in writing.
    // data[] = {0x34, 0x12}
    //

    // Load Data Value to Appropriate Address
    p_info_page_buf[data_index]		= (unsigned char) (value & 0x00FF);
    p_info_page_buf[data_index+1]	= (unsigned char)((value & 0xFF00) >> 8);
    DEBUG_PRINTF("%s: address: 0x%04x, value: %d (0x%04x), info_page_buf[%d] = {%02x %02x}.\r\n", \
                 __func__, address, value, value, data_index, p_info_page_buf[data_index], p_info_page_buf[data_index + 1]);

    // Success
    err = TP_SUCCESS;

SET_INFO_PAGE_VALUE_EXIT:
    return err;
}

int get_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short *p_value)
{
    int err = TP_SUCCESS;
    unsigned short data_index	= 0, // Memory Address Offset
                   data_value	= 0;

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_VALUE_EXIT;
    }

    // Address
    if(address < ELAN_INFO_MEMORY_PAGE_1_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%04x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_VALUE_EXIT;
    }

    // Value Buffer
    if(p_value == NULL)
    {
        ERROR_PRINTF("%s: NULL Value Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_VALUE_EXIT;
    }

    //
    // Get Value
    //

    // Data Index
    data_index = (address - ELAN_INFO_MEMORY_PAGE_1_ADDR) * 2 /* unit: byte */;

    //
    // [Note] 2022/08/03
    // Ordering with big-endian, in reading.
    // data[] = {0x12, 0x34}
    // Swap data in reading!
    //

    // Data Value
    data_value = REVERSE_TWO_BYTE_ARRAY_TO_WORD(&p_info_page_buf[data_index]);
    DEBUG_PRINTF("%s: MEM[0x%04x] = {0x%x 0x%x} => 0x%04x (%d).\r\n", __func__, \
                 address, p_info_page_buf[data_index], p_info_page_buf[data_index + 1], \
                 data_value, data_value);

    // Load Data Value to Input Value Buffer
    *p_value = data_value;

    // Success
    err = TP_SUCCESS;

GET_INFO_PAGE_VALUE_EXIT:
    return err;
}

int set_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short value)
{
    int err = TP_SUCCESS;
    unsigned short data_index	= 0; // Memory Address Offset
    unsigned short hex_value	= 0;
    char hex_value_string[8]	= {0}; // Format: "0xabcd"

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto SET_INFO_PAGE_DATA_EXIT;
    }

    // Address
    if(address < ELAN_INFO_MEMORY_PAGE_1_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%04x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto SET_INFO_PAGE_DATA_EXIT;
    }

    // Data Index
    data_index = (address - ELAN_INFO_MEMORY_PAGE_1_ADDR) * 2 /* unit: byte */;

    // Data Value
    sprintf(hex_value_string, "0x%d", value);
    hex_value = (unsigned short)strtoul(hex_value_string, NULL, 0 /* determined by the format of hex_value_string */);

    //
    // [Note] 2022/08/03
    // Ordering with little-endian, in writing.
    // data[] = {0x34, 0x12}
    //

    // Load Data Value to Appropriate Address
    p_info_page_buf[data_index]		= (unsigned char) (hex_value & 0x00FF);
    p_info_page_buf[data_index+1]	= (unsigned char)((hex_value & 0xFF00) >> 8);
    DEBUG_PRINTF("%s: address: 0x%04x, value: %d (0x%04x), info_page_buf[%d] = {%02x %02x}.\r\n", \
                 __func__, address, value, value, data_index, p_info_page_buf[data_index], p_info_page_buf[data_index + 1]);

    // Success
    err = TP_SUCCESS;

SET_INFO_PAGE_DATA_EXIT:
    return err;
}

int get_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned short *p_value)
{
    int err = TP_SUCCESS;
    unsigned short data_index	= 0, // Memory Address Offset
                   data_value	= 0,
                   int_value	= 0;
    char int_value_string[8]	= {0}; // Format: "abcd"

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_DATA_EXIT;
    }

    // Address
    if(address < ELAN_INFO_MEMORY_PAGE_1_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%04x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_DATA_EXIT;
    }

    // Value Buffer
    if(p_value == NULL)
    {
        ERROR_PRINTF("%s: NULL Value Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_DATA_EXIT;
    }

    //
    // Get Data Value
    //

    // Data Index
    data_index = (address - ELAN_INFO_MEMORY_PAGE_1_ADDR) * 2 /* unit: byte */;

    //
    // [Note] 2022/08/03
    // Ordering with big-endian, in reading.
    // data[] = {0x12, 0x34}
    // Swap data in reading!
    //

    // Data Value
    //data_value = TWO_BYTE_ARRAY_TO_WORD(&p_info_page_buf[data_index]);
    data_value = REVERSE_TWO_BYTE_ARRAY_TO_WORD(&p_info_page_buf[data_index]);

    // Hex. to Int.
    sprintf(int_value_string, "%x", data_value);
    int_value = (unsigned short)strtoul(int_value_string, NULL, 10);
    DEBUG_PRINTF("%s: MEM[0x%04x] = {0x%x 0x%x} => 0x%04x => %d.\r\n", __func__, \
                 address, p_info_page_buf[data_index], p_info_page_buf[data_index + 1], \
                 data_value, int_value);

    // Load Integer Value to Input Value Buffer
    *p_value = int_value;

    // Success
    err = TP_SUCCESS;

GET_INFO_PAGE_DATA_EXIT:
    return err;
}

int set_info_page_data_bytes(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned char high_byte, unsigned char low_byte)
{
    int err = TP_SUCCESS;
    unsigned short data_index			= 0; // Memory Address Offset
    unsigned short high_byte_hex_value	= 0,
                   low_byte_hex_value	= 0;
    char hex_value_string[8]			= {0}; // Format: "0xab"

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto SET_INFO_PAGE_DATA_BYTES_EXIT;
    }

    // Address
    if(address < ELAN_INFO_MEMORY_PAGE_1_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%04x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto SET_INFO_PAGE_DATA_BYTES_EXIT;
    }

    // Data Index
    data_index = (address - ELAN_INFO_MEMORY_PAGE_1_ADDR) * 2 /* unit: byte */;

    // Data Value (High Byte)
    sprintf(hex_value_string, "0x%d", high_byte);
    high_byte_hex_value = (unsigned short)strtoul(hex_value_string, NULL, 0 /* determined by the format of hex_value_string */);

    // Data Value (Low Byte)
    sprintf(hex_value_string, "0x%d", low_byte);
    low_byte_hex_value = (unsigned short)strtoul(hex_value_string, NULL, 0 /* determined by the format of hex_value_string */);

    //
    // [Note] 2022/08/03
    // Ordering with little-endian, in writing.
    // data[] = {0x34, 0x12}
    //

    // Load Data Value to Appropriate Address
    p_info_page_buf[data_index]		= low_byte_hex_value;
    p_info_page_buf[data_index+1]	= high_byte_hex_value;
    DEBUG_PRINTF("%s: address: 0x%04x, high_byte: %d (0x%02x), low_byte: %d (0x%02x), info_page_buf[%d] = 0x%02x, info_page_buf[%d] = 0x%02x.\r\n", \
                 __func__, address, high_byte, high_byte, low_byte, low_byte, data_index, p_info_page_buf[data_index], \
                 (data_index + 1), p_info_page_buf[data_index + 1]);

    // Success
    err = TP_SUCCESS;

SET_INFO_PAGE_DATA_BYTES_EXIT:
    return err;
}

int get_info_page_data_bytes(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned short address, unsigned char *p_high_byte, unsigned char *p_low_byte)
{
    int err = TP_SUCCESS;
    unsigned short data_index			= 0; // Memory Address Offset
    char int_value_string[4]			= {0}; // Format: "ab"
    unsigned char low_byte_value		= 0,
                  high_byte_value		= 0,
                  low_byte_int_value	= 0,
                  high_byte_int_value	= 0; // Range: 0 ~ 59

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_DATA_BYTES_EXIT;
    }

    // Address
    if(address < ELAN_INFO_MEMORY_PAGE_1_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%04x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_DATA_BYTES_EXIT;
    }

    // Value Buffers
    if((p_high_byte == NULL) || (p_low_byte == NULL))
    {
        ERROR_PRINTF("%s: Invalid Value Buffers! (p_high_byte=0x%p, p_low_byte=0x%p)\r\n", __func__, p_high_byte, p_low_byte);
        err = TP_ERR_INVALID_PARAM;
        goto GET_INFO_PAGE_DATA_BYTES_EXIT;
    }

    //
    // Get Data Bytes
    //

    // Data Index
    data_index = (address - ELAN_INFO_MEMORY_PAGE_1_ADDR) * 2 /* unit: byte */;

    //
    // [Note] 2022/08/03
    // Ordering with big-endian, in reading.
    // data[] = {0x12, 0x34}
    // Swap data in reading!
    //

    // High Byte Value
    high_byte_value	= p_info_page_buf[data_index];
    sprintf(int_value_string, "%x", high_byte_value);
    high_byte_int_value = (unsigned char)strtoul(int_value_string, NULL, 10);

    // Low Byte Value
    low_byte_value	= p_info_page_buf[data_index + 1];
    sprintf(int_value_string, "%x", low_byte_value);
    low_byte_int_value = (unsigned char)strtoul(int_value_string, NULL, 10);

    DEBUG_PRINTF("%s: MEM[0x%04x] = {0x%02x 0x%02x} => {%d %d}.\r\n", __func__, \
                 address, high_byte_value, low_byte_value, high_byte_int_value, low_byte_int_value);

    // Load Integer Values to Input Value Buffers
    *p_high_byte	= high_byte_int_value;
    *p_low_byte		= low_byte_int_value;

    // Success
    err = TP_SUCCESS;

GET_INFO_PAGE_DATA_BYTES_EXIT:
    return err;
}

int get_update_info(unsigned char *p_info_page_buf, size_t info_page_buf_size, struct update_info *p_update_info, size_t update_info_size)
{
    int err = TP_SUCCESS;
    unsigned short update_counter				= 0,
                   last_update_time_year		= 0;
    unsigned char  last_update_time_month		= 0,
                   last_update_time_day			= 0,
                   last_update_time_hour		= 0,
                   last_update_time_minute		= 0;
    struct update_info last_update_info;

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_UPDATE_INFO_EXIT;
    }

    // Update Info. & Update Info. Size
    if((p_update_info == NULL) || (update_info_size < sizeof(struct update_info)))
    {
        ERROR_PRINTF("%s: Invalid Update Info.! (p_update_info=0x%p, update_info_size=%ld)\r\n", \
                     __func__, p_update_info, update_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_UPDATE_INFO_EXIT;
    }

    //
    // Get Update Inforamtion
    //

    // [Note] 2019/04/12
    // Ordering with Little-Endian in Reading

    // Update Counter
    err = get_info_page_value(p_info_page_buf, info_page_buf_size, ELAN_UPDATE_COUNTER_ADDR, &update_counter);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Update Counter (Address: 0x%04x)! err=0x%x.\r\n", __func__, \
                     ELAN_UPDATE_COUNTER_ADDR, err);
        goto GET_UPDATE_INFO_EXIT;
    }
    if(update_counter == 0xFFFF)
        update_counter = 0;

    // Last Update Time (Year)
    err = get_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_LAST_UPDATE_TIME_YEAR_ADDR, &last_update_time_year);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Year (Address: 0x%04x)! err=0x%x.\r\n", __func__, \
                     ELAN_LAST_UPDATE_TIME_YEAR_ADDR, err);
        goto GET_UPDATE_INFO_EXIT;
    }

    // Last Update Time (Month & Day)
    err = get_info_page_data_bytes(p_info_page_buf, info_page_buf_size, ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR, &last_update_time_month, &last_update_time_day);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Month & Day (Address: 0x%04x)! err=0x%x.\r\n", __func__, \
                     ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR, err);
        goto GET_UPDATE_INFO_EXIT;
    }

    // Last Update Time (Hour & Minute)
    err = get_info_page_data_bytes(p_info_page_buf, info_page_buf_size, ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR, &last_update_time_hour, &last_update_time_minute);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Hour & Minute (Address: 0x%04x)! err=0x%x.\r\n", __func__, \
                     ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR, err);
        goto GET_UPDATE_INFO_EXIT;
    }

    // Setup Last Update Info. Container
    last_update_info.update_counter				= update_counter;
    last_update_info.last_update_time.Year		= last_update_time_year;
    last_update_info.last_update_time.Month		= last_update_time_month;
    last_update_info.last_update_time.Day		= last_update_time_day;
    last_update_info.last_update_time.Hour		= last_update_time_hour;
    last_update_info.last_update_time.Minute	= last_update_time_minute;

    // Load Last Update Info. to Input Buffer
    memcpy(p_update_info, &last_update_info, sizeof(last_update_info));

    // Success
    err = TP_SUCCESS;

GET_UPDATE_INFO_EXIT:
    return err;
}

int update_info_page(struct update_info *p_update_info, size_t update_info_size, unsigned char *p_info_page_buf, size_t info_page_buf_size)
{
    int err = TP_SUCCESS;

    //
    // Validate Arguments
    //

    // Update Info. & Update Info. Size
    if((p_update_info == NULL) || (update_info_size < sizeof(struct update_info)))
    {
        ERROR_PRINTF("%s: Invalid Update Info.! (p_update_info=0x%p, update_info_size=%ld)\r\n", \
                     __func__, p_update_info, update_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto UPDATE_INFO_PAGE_EXIT;
    }

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto UPDATE_INFO_PAGE_EXIT;
    }

    //
    // Get Update Inforamtion
    //

    // Update Counter
    err = set_info_page_value(p_info_page_buf, info_page_buf_size, ELAN_UPDATE_COUNTER_ADDR, p_update_info->update_counter);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Update Counter (Address: 0x%04x, Value: 0x%04x)! err=0x%x.\r\n", __func__, \
                     ELAN_UPDATE_COUNTER_ADDR, p_update_info->update_counter, err);
        goto UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Year)
    err = set_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_LAST_UPDATE_TIME_YEAR_ADDR, p_update_info->last_update_time.Year);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Year (Address: 0x%04x, Value: 0x%04x)! err=0x%x.\r\n", __func__, \
                     ELAN_LAST_UPDATE_TIME_YEAR_ADDR, p_update_info->last_update_time.Year, err);
        goto UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Month & Day)
    err = set_info_page_data_bytes(p_info_page_buf, info_page_buf_size, ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR, \
                                   p_update_info->last_update_time.Month, p_update_info->last_update_time.Day);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Month & Day (Address: 0x%04x, high_byte: 0x%02x, low_byte: 0x%02x)! err=0x%x.\r\n", __func__, \
                     ELAN_LAST_UPDATE_TIME_MONTH_DAY_ADDR, p_update_info->last_update_time.Month, p_update_info->last_update_time.Day, err);
        goto UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Hour & Minute)
    err = set_info_page_data_bytes(p_info_page_buf, info_page_buf_size, ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR, \
                                   p_update_info->last_update_time.Hour, p_update_info->last_update_time.Minute);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Hour & Minute (Address: 0x%04x, high_byte: 0x%02x, low_byte: 0x%02x)! err=0x%x.\r\n", __func__, \
                     ELAN_LAST_UPDATE_TIME_HOUR_MINUTE_ADDR, p_update_info->last_update_time.Hour, p_update_info->last_update_time.Minute, err);
        goto UPDATE_INFO_PAGE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

UPDATE_INFO_PAGE_EXIT:
    return err;
}

int get_and_update_info_page(unsigned char solution_id, unsigned char *p_info_page_buf, size_t info_page_buf_size)
{
    int err = TP_SUCCESS;
    unsigned short memory_page_address = 0;
    unsigned char info_mem_page_buf[ELAN_MEMORY_PAGE_SIZE]	= {0},
                  fw_info_page_buf[ELAN_FIRMWARE_PAGE_SIZE] = {0},
                  fw_info_page_data_buf[ELAN_FIRMWARE_PAGE_DATA_SIZE] = {0};
    time_t cur_time;
    struct tm *time_info;
    struct update_info last_update_info;

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    //
    // Get Update Inforamtion
    //

    // Get Information Memory Page Data
    err = get_info_page_with_error_retry(info_mem_page_buf, sizeof(info_mem_page_buf), ERROR_RETRY_COUNT);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Information Page! err=0x%x.\r\n", __func__, err);
        goto GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    // Initialize eKTL FW Information Page Data Buffer
    memcpy(fw_info_page_data_buf, info_mem_page_buf, sizeof(info_mem_page_buf));

    // Parse Update Info. from Information Memory Page Data
    err = get_update_info(info_mem_page_buf, sizeof(info_mem_page_buf), &last_update_info, sizeof(last_update_info));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Parse Update Information! err=0x%x.\r\n", __func__, err);
        goto GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    DEBUG_PRINTF("%s: Previous Update Information - Update Counter=%d, Last Update Time: %04d/%02d/%02d %02d:%02d.\r\n", __func__, \
                 last_update_info.update_counter, \
                 last_update_info.last_update_time.Year, last_update_info.last_update_time.Month, last_update_info.last_update_time.Day, \
                 last_update_info.last_update_time.Hour, last_update_info.last_update_time.Minute);

    //
    // Refresh Update Inforamtion
    //

    // Increase Update Counter
    last_update_info.update_counter++;

    // Acquire Current Time
    time(&cur_time);
    time_info = localtime(&cur_time);

    // Update Last Update Time
    last_update_info.last_update_time.Year		= 1900 + time_info->tm_year;
    last_update_info.last_update_time.Month		= 1 + time_info->tm_mon;
    last_update_info.last_update_time.Day		= time_info->tm_mday;
    last_update_info.last_update_time.Hour		= time_info->tm_hour;
    last_update_info.last_update_time.Minute	= time_info->tm_min;

    DEBUG_PRINTF("%s: Current Update Information - Update Counter=%d, Last Update Time: %04d/%02d/%02d %02d:%02d.\r\n", __func__, \
                 last_update_info.update_counter, \
                 last_update_info.last_update_time.Year, last_update_info.last_update_time.Month, last_update_info.last_update_time.Day, \
                 last_update_info.last_update_time.Hour, last_update_info.last_update_time.Minute);

    //
    // Update Inforamtion Page Data
    //

    // Update eKTL Inforamtion Page Data
    err = update_info_page(&last_update_info, sizeof(last_update_info), fw_info_page_data_buf, sizeof(fw_info_page_data_buf));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Update Information Page Data! err=0x%x.\r\n", __func__, err);
        goto GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    //
    // Setup Information Page
    //

    // [Note] 2022/03/31
    // Support Info. Memory Space of Gen5 Series.
    // Since we start to adopt Gen5 touch into chrome project, momory space of 32k flash should be taked into consideration.
    // Thus we have two type of information memory spaces:
    // Gen5 Touch (with 32k flash): info. memory space starts from 0x8040.
    // Gen6 Touch (with 64k flash): info. memory space starts from 0x0040.

    // Configure Memory Page Address
    DEBUG_PRINTF("%s: solution_id=0x%02x.\r\n", __func__, solution_id);
    if (/* 63XX Solution */
        (solution_id == SOLUTION_ID_EKTH6315x1) || \
        (solution_id == SOLUTION_ID_EKTH6315x2) || \
        (solution_id == SOLUTION_ID_EKTH6315to5015M) || \
        (solution_id == SOLUTION_ID_EKTH6315to3915P) || \
        /* 73XX Solution */
        (solution_id == SOLUTION_ID_EKTH7315x1) || \
        (solution_id == SOLUTION_ID_EKTH7315x2) || \
        (solution_id == SOLUTION_ID_EKTH7318x1))
        memory_page_address = ELAN_INFO_PAGE_WRITE_MEMORY_ADDR;	// 63XX or 73XX: memory_page_addr=0x0040
    else // 53XX Solution
        memory_page_address = ELAN_INFO_MEMORY_PAGE_1_ADDR;		// 53XX: memory_page_addr=0x8040

    // Setup Information Page
    err = create_firmware_page(memory_page_address, \
                               fw_info_page_data_buf, sizeof(fw_info_page_data_buf), \
                               fw_info_page_buf, sizeof(fw_info_page_buf));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Setup Information Page! err=0x%x.\r\n", __func__, err);
        goto GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    // Load Information Page Data to Input Buffer
    memcpy(p_info_page_buf, fw_info_page_buf, sizeof(fw_info_page_buf));

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
        ERROR_PRINTF("%s: Fail to Send Slave Address! err=0x%x.\r\n", __func__, err);
        goto CHECK_SLAVE_ADDRESS_EXIT;
    }

    // Read Slave Address
    err = read_data(&slave_addr, 1, ELAN_READ_DATA_TIMEOUT_MSEC);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Read Slave Address! err=0x%x.\r\n", __func__, err);
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
            ERROR_PRINTF("%s: [Normal IAP] Fail to Write Flash Key! err=0x%x.\r\n", __func__, err);
            goto SWITCH_TO_BOOT_CODE_EXIT;
        }

        // Enter IAP Mode
        err = send_enter_iap_command();
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: [Normal IAP] Fail to Enter IAP Mode! err=0x%x.\r\n", __func__, err);
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
            ERROR_PRINTF("%s: [Recovery IAP] Fail to Write Flash Key! err=0x%x.\r\n", __func__, err);
            goto SWITCH_TO_BOOT_CODE_EXIT;
        }
    }

    // wait 15ms
    usleep(15*1000);

    // Check Slave Address
    err = check_slave_address();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Check Slave Address! err=0x%x.\r\n", __func__, err);
        goto SWITCH_TO_BOOT_CODE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

SWITCH_TO_BOOT_CODE_EXIT:
    return err;
}

int create_firmware_page(unsigned int mem_page_address, unsigned char *p_fw_page_data_buf, size_t fw_page_data_buf_size, unsigned char *p_fw_page_buf, size_t fw_page_buf_size)
{
    int err = TP_SUCCESS,
        page_data_index = 0;
    unsigned short page_data		= 0,
                   page_checksum	= 0;
    unsigned char firmware_page_buf[ELAN_FIRMWARE_PAGE_SIZE] = {0};

    //
    // Validate Arguments
    //

    // FW Page Buffer Page Data
    if((p_fw_page_data_buf == NULL) || (fw_page_data_buf_size < ELAN_FIRMWARE_PAGE_DATA_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_fw_page_data_buf=0x%p, fw_page_data_buf_size=%ld)\r\n", __func__, p_fw_page_data_buf, fw_page_data_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto CREATE_FIRMWARE_PAGE_EXIT;
    }

    // FW Page Buffer Page
    if((p_fw_page_buf == NULL) || (fw_page_buf_size < ELAN_FIRMWARE_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_fw_page_buf=0x%p, fw_page_buf_size=%ld)\r\n", __func__, p_fw_page_buf, fw_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto CREATE_FIRMWARE_PAGE_EXIT;
    }

    //
    // Setup Firmware Page Buffer
    //

    // Set Page Address
    firmware_page_buf[0] = (unsigned char) (mem_page_address & 0x00FF);			// Low  Byte of Address
    firmware_page_buf[1] = (unsigned char)((mem_page_address & 0xFF00) >> 8);	// High Byte of Address

    // Set Page Data
    memcpy(&firmware_page_buf[2], p_fw_page_data_buf, fw_page_data_buf_size);

    // Compute Page Checksum
    for(page_data_index = 0; page_data_index < (ELAN_FIRMWARE_PAGE_SIZE - 2); page_data_index += 2)
    {
        // Get Page Data
        page_data = (firmware_page_buf[page_data_index + 1] << 8) | firmware_page_buf[page_data_index];

        // If page address is 0x0040, replace it with 0x8040.
        if((page_data_index == 0) && (page_data == ELAN_INFO_PAGE_WRITE_MEMORY_ADDR)) // page_data[0]=0x0040
            page_data = ELAN_INFO_MEMORY_PAGE_1_ADDR;

        // Update Checksum
        page_checksum += page_data;
    }
    DEBUG_PRINTF("%s: Checksum=0x%04x.\r\n", __func__, page_checksum);

    // Set Page CheckSum
    firmware_page_buf[ELAN_FIRMWARE_PAGE_SIZE - 2] = (unsigned char) (page_checksum & 0x00FF);			// Low  Byte of Checksum
    firmware_page_buf[ELAN_FIRMWARE_PAGE_SIZE - 1] = (unsigned char)((page_checksum & 0xFF00) >> 8);	// High Byte of Checksum

    // Copy Info. Page Data to Input Buffer
    memcpy(p_fw_page_buf, firmware_page_buf, sizeof(firmware_page_buf));

    // Success
    err = TP_SUCCESS;

CREATE_FIRMWARE_PAGE_EXIT:
    return err;
}

// Page Data
int write_firmware_page(unsigned char *p_fw_page_buf, int fw_page_buf_size)
{
    int err = TP_SUCCESS,
        frame_index = 0,
        frame_count = 0,
        frame_data_len = 0,
        start_index = 0;
    unsigned char fw_page_block_buf[ELAN_FIRMWARE_PAGE_SIZE * 30] = {0};

    // Valid Page Buffer
    if(p_fw_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL Page Buffer Pointer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto WRITE_FIRMWARE_PAGE_EXIT;
    }

    // Validate Page Buffer Size
    if((fw_page_buf_size == 0) || (fw_page_buf_size > (ELAN_FIRMWARE_PAGE_SIZE * 30)))
    {
        ERROR_PRINTF("%s: Invalid Page Buffer Size: %d.\r\n", __func__, fw_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto WRITE_FIRMWARE_PAGE_EXIT;
    }

    // Copy Page Data from Input Page Buffer to Temp Page Buffer
    memcpy(fw_page_block_buf, p_fw_page_buf, fw_page_buf_size);

    // Get Frame Count
    frame_count = (fw_page_buf_size / ELAN_I2CHID_PAGE_FRAME_SIZE) +
                  ((fw_page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE) != 0);

    // Write Page Data with Frames
    for(frame_index = 0; frame_index < frame_count; frame_index++)
    {
        if((frame_index == (frame_count - 1)) && ((fw_page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE) > 0)) // The Last Frame
            frame_data_len = fw_page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE;
        else
            frame_data_len = ELAN_I2CHID_PAGE_FRAME_SIZE;

        // Write Frame Data
        err = write_frame_data(start_index, frame_data_len, &fw_page_block_buf[start_index], frame_data_len);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Update Page from 0x%x (Length %d)! err=0x%x.\r\n", __func__, start_index, frame_data_len, err);
            goto WRITE_FIRMWARE_PAGE_EXIT;
        }

        // Update Start Index to Next Frame
        start_index += frame_data_len;
    }

    // Request Flash Write
    err = send_flash_write_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Request Flash Write! err=0x%x.\r\n", __func__, err);
        goto WRITE_FIRMWARE_PAGE_EXIT;
    }

    // Wait for FW Writing Flash
    if(fw_page_buf_size == (ELAN_FIRMWARE_PAGE_SIZE * 30)) // 30 Page Block
        usleep(360 * 1000); // wait 12ms * 30
    else
        usleep(15 * 1000); // wait 15ms

    // Receive Response of Flash Write
    err = receive_flash_write_response();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive Flash Write! err=0x%x.\r\n", __func__, err);
        goto WRITE_FIRMWARE_PAGE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

WRITE_FIRMWARE_PAGE_EXIT:
    return err;
}

// ROM Data
int get_rom_data(unsigned short addr, bool recovery, unsigned short *p_data)
{
    int err = TP_SUCCESS;
    unsigned short bc_bc_version = 0,
                   word_data = 0;
    unsigned char  hello_packet = 0,
                   solution_id = 0,
                   bc_version_high_byte = 0;

    // Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GET_ROM_DATA_EXIT;
    }

    if(!recovery) // Normal Mode
    {
        // Solution ID
        err = get_solution_id(&solution_id);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get Solution ID! err=0x%x.\r\n", __func__, err);
            goto GET_ROM_DATA_EXIT;
        }
        DEBUG_PRINTF("%s: [Normal Mode] Solution ID: 0x%02x.\r\n", __func__, solution_id);
    }
    else // Recovery Mode
    {
        // BC Version (Recovery Mode)
        err = get_hello_packet_bc_version(&hello_packet, &bc_bc_version);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get BC Version (Recovery Mode)! err=0x%x.\r\n", __func__, err);
            goto GET_ROM_DATA_EXIT;
        }
        DEBUG_PRINTF("%s: [Recovery Mode] BC Version: 0x%04x.\r\n", __func__, bc_bc_version);
    }

    /* Read Data from ROM */
    err = send_read_rom_data_command(addr, recovery, (recovery) ? bc_version_high_byte : solution_id);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Read ROM Data Command! err=0x%x.\r\n", __func__, err);
        goto GET_ROM_DATA_EXIT;
    }

    err = receive_rom_data(&word_data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive ROM Data! err=0x%x.\r\n", __func__, err);
        goto GET_ROM_DATA_EXIT;
    }

    *p_data = word_data;
    err = TP_SUCCESS;

GET_ROM_DATA_EXIT:
    return err;
}

// Remark ID
int read_remark_id(bool recovery)
{
    int err = TP_SUCCESS;
    unsigned short remark_id = 0;

    // Get Remark ID from ROM
    err = get_rom_data(ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR, recovery, &remark_id);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Read Remark ID from ROM! err=0x%x.\r\n", __func__, err);
        goto READ_REMARK_ID_EXIT;
    }
    DEBUG_PRINTF("Remark ID: %04x.\r\n", remark_id);

    err = TP_SUCCESS;

READ_REMARK_ID_EXIT:
    return err;
}

// Bulk ROM Data
int get_bulk_rom_data(unsigned short addr, unsigned short *p_data)
{
    int err = TP_SUCCESS;
    unsigned short data = 0;

    // Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GET_BULK_ROM_DATA_EXIT;
    }

    err = send_show_bulk_rom_data_command(addr);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Show Bulk ROM Data Command (addr=0x%04x)! err=0x%x.\r\n", __func__, addr, err);
        goto GET_BULK_ROM_DATA_EXIT;
    }

    err = receive_bulk_rom_data(&data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive Bulk ROM Data! err=0x%x.\r\n", __func__, err);
        goto GET_BULK_ROM_DATA_EXIT;
    }

    *p_data = data;
    err = TP_SUCCESS;

GET_BULK_ROM_DATA_EXIT:
    return err;
}

// Information FWID
int read_info_fwid(unsigned short *p_info_fwid, bool recovery)
{
    int err = TP_SUCCESS;
    unsigned short info_fwid = 0;

    // Check if Parameter Invalid
    if (p_info_fwid == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_info_fwid=0x%p)\r\n", __func__, p_info_fwid);
        err = TP_ERR_INVALID_PARAM;
        goto READ_INFO_FWID_EXIT;
    }

    // Get FWID from ROM
    if(recovery) // Recovery Mode
    {
        // Read FWID from ROM (with bulk rom data command)
        // [Note] Paul @ 20191025
        // Since Read ROM Command only supported by main code,
        //   we can only use Show Bulk ROM Data Command (0x59) in recovery mode (boot code stage).
        err = get_bulk_rom_data(ELAN_INFO_ROM_FWID_MEMORY_ADDR, &info_fwid);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get Bulk ROM Data Command! err=0x%x.\r\n", __func__, err);
            goto READ_INFO_FWID_EXIT;
        }
    }
    else // Normal Mode
    {
        /* Read FWID from ROM */
        // [Note] Paul @ 20191025
        // In normal mode, just use Read ROM Command (0x96).
        err = get_rom_data(ELAN_INFO_ROM_FWID_MEMORY_ADDR, recovery, &info_fwid);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get ROM Data! err=0x%x.\r\n", __func__, err);
            goto READ_INFO_FWID_EXIT;
        }
    }

    *p_info_fwid = info_fwid;
    err = TP_SUCCESS;

READ_INFO_FWID_EXIT:
    return err;
}

