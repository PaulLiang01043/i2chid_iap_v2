/** @file

  Implementation of Function Utility for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsI2chidUtility.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include "I2CHIDLinuxGet.h"
#include "ElanGen8TsI2chidUtility.h"

/***************************************************
 * TP Functions
 ***************************************************/

// Test Version
int gen8_read_test_version_data(void)
{
    int err = TP_SUCCESS;
    unsigned short test_version = 0;
    unsigned char cmd_data[4] = {0};

    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to receive Test Version data, err=0x%x.\r\n", err);
        goto GEN8_READ_TEST_VERSION_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

    /* Check if Data is for Test Version */
    if ((cmd_data[0] == 0x52) && (((cmd_data[1] & 0xf0) >> 4) == 0xe))
    {
        test_version = (cmd_data[2] << 8) | cmd_data[3];
        printf("Test Version: %04x\r\n", test_version);
    }

    err = TP_SUCCESS;

GEN8_READ_TEST_VERSION_DATA_EXIT:
    return err;
}

int gen8_get_test_version_data(unsigned short *p_test_version)
{
    int err = TP_SUCCESS;
    unsigned short test_version = 0;
    unsigned char cmd_data[4] = {0};

    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to receive Test Version data, err=0x%x.\r\n", err);
        goto GEN8_GET_TEST_VERSION_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

    /* Check if Data is for Test Version */
    if ((cmd_data[0] != 0x52) || (((cmd_data[1] & 0xf0) >> 4) != 0xe))
    {
        err = TP_ERR_DATA_PATTERN;
        ERROR_PRINTF("Invalid Data Format (%02x %02x), err=0x%x.\r\n", cmd_data[0], cmd_data[1], err);
        goto GEN8_GET_TEST_VERSION_DATA_EXIT;
    }

    test_version = (cmd_data[2] << 8) | cmd_data[3];
    DEBUG_PRINTF("test_version: 0x%04x\r\n", test_version);

    *p_test_version = test_version;
    err = TP_SUCCESS;

GEN8_GET_TEST_VERSION_DATA_EXIT:
    return err;
}

// ROM Data
int gen8_send_read_rom_data_command(unsigned int addr, unsigned char data_len)
{
    int err = TP_SUCCESS;
    unsigned char new_read_rom_data_cmd[10] =  {0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /* Read 32-bit RAM/ROM Data Command */;

    // Check if Parameter Invalid
    if ((data_len != 1) && (data_len != 2) && (data_len != 4))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (data_len=%d)\r\n", __func__, data_len);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_SEND_READ_ROM_DATA_COMMAND_EXIT;
    }

    /* Set Address & Length */

    // Set Length
    new_read_rom_data_cmd[1] = data_len;

    // Set Address
    new_read_rom_data_cmd[2] = (unsigned char)((addr & 0xFF000000) >> 24); //ADDR_3, ex: 0x00
    new_read_rom_data_cmd[3] = (unsigned char)((addr & 0x00FF0000) >> 16); //ADDR_2, ex: 0x04
    new_read_rom_data_cmd[4] = (unsigned char)((addr & 0x0000FF00) >>  8); //ADDR_1, ex: 0x00
    new_read_rom_data_cmd[5] = (unsigned char)((addr & 0x000000FF)		); //ADDR_0, ex: 0x00

    /* Send Read 32-bit RAM/ROM Data Command */
    DEBUG_PRINTF("cmd: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x.\r\n", \
                 new_read_rom_data_cmd[0], new_read_rom_data_cmd[1], \
                 new_read_rom_data_cmd[2], new_read_rom_data_cmd[3], new_read_rom_data_cmd[4], new_read_rom_data_cmd[5], \
                 new_read_rom_data_cmd[6], new_read_rom_data_cmd[7], new_read_rom_data_cmd[8], new_read_rom_data_cmd[9]);
    err = write_cmd(new_read_rom_data_cmd, sizeof(new_read_rom_data_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to send Read 32-bit RAM/ROM Data Command! err=0x%x.\r\n", __func__, err);
        goto GEN8_SEND_READ_ROM_DATA_COMMAND_EXIT;
    }

    // Success
    err = TP_SUCCESS;

GEN8_SEND_READ_ROM_DATA_COMMAND_EXIT:
    return err;
}

int gen8_receive_rom_data(unsigned int *p_rom_data)
{
    int err = TP_SUCCESS;
    unsigned char cmd_data[10] = {0};
    unsigned short rom_data = 0;

    // Check if Parameter Invalid
    if (p_rom_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_rom_data=0x%p)\r\n", __func__, p_rom_data);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_RECEIVE_ROM_DATA_EXIT;
    }

    // Read 10-byte Command Data
    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if(err != TP_SUCCESS) // Error or Timeout
    {
        ERROR_PRINTF("Fail to receive ROM data! err=0x%x.\r\n", err);
        goto GEN8_RECEIVE_ROM_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x.\r\n", \
                 cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3], cmd_data[4], cmd_data[5], \
                 cmd_data[6], cmd_data[7], cmd_data[8], cmd_data[9]);

    /* Check if data invalid */
    //if ((cmd_data[0] != 0x95) || (cmd_data[1] != 0x04))
    if ( (cmd_data[0] != 0x95) || \
            ((cmd_data[1] != 0x01) && (cmd_data[1] != 0x02) && (cmd_data[1] != 0x04)))
    {
        err = TP_ERR_DATA_PATTERN;
        ERROR_PRINTF("%s: Data Format Invalid! (cmd_data[0]=0x%02x, cmd_data[1]=0x%02x), err=0x%x.\r\n", \
                     __func__, cmd_data[0], cmd_data[1], err);
        goto GEN8_RECEIVE_ROM_DATA_EXIT;
    }

    // Load 4-byte ROM Data to Input Buffer
    rom_data = (unsigned int)((cmd_data[6] << 24) | (cmd_data[7] << 16) | (cmd_data[8] << 8) | cmd_data[9]);
    //DEBUG_PRINTF("ROM Data: 0x%08x.\r\n", rom_data);
    *p_rom_data = rom_data;

    // Success
    err = TP_SUCCESS;

GEN8_RECEIVE_ROM_DATA_EXIT:
    return err;
}

// IAP Mode
int send_gen8_write_flash_key_command(void)
{
    int err = TP_SUCCESS;
    unsigned char gen8_write_flash_key_cmd[10] = {0x54, 0xc0, 0xcd, 0xab, 0x34, 0x84, 0x01, 0x67, 0x94, 0x81};

    /* Send Gen8 Write Flash Key Command */
    DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x.\r\n", \
                 gen8_write_flash_key_cmd[0], gen8_write_flash_key_cmd[1], gen8_write_flash_key_cmd[2], gen8_write_flash_key_cmd[3], \
                 gen8_write_flash_key_cmd[4], gen8_write_flash_key_cmd[5], gen8_write_flash_key_cmd[6], gen8_write_flash_key_cmd[7], \
                 gen8_write_flash_key_cmd[8], gen8_write_flash_key_cmd[9]);
    err = write_cmd(gen8_write_flash_key_cmd, sizeof(gen8_write_flash_key_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to send Gen8 Write Flash Key command! err=0x%x.\r\n", err);
    }

    return err;
}

// Erase Flash Section
int send_erase_flash_section_command(unsigned int address, unsigned short page_count)
{
    int err = TP_SUCCESS;
    unsigned char hid_frame_data[ELAN_I2CHID_OUTPUT_BUFFER_SIZE] = {0};

    // Valid Page Count to Erase
    if(page_count == 0)
    {
        ERROR_PRINTF("%s: Invalid Page Count (0)!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto SEND_ERASE_FLASH_SECTION_COMMMAND_EXIT;
    }

    // Clear Frame Data
    memset(hid_frame_data, 0, sizeof(hid_frame_data));

    // Add header of vendor command to frame data
    hid_frame_data[0] = ELAN_HID_OUTPUT_REPORT_ID;						// 0x03
    hid_frame_data[1] = 0x20;
    hid_frame_data[2] = (unsigned char) (address & 0x000000FF);			// LSB (Byte 0) of Address		//ex:00
    hid_frame_data[3] = (unsigned char)((address & 0x0000FF00) >>  8);	//      Byte 1  of Address		//ex:F8
    hid_frame_data[4] = (unsigned char)((address & 0x00FF0000) >> 16);	//      Byte 2  of Address		//ex:03
    hid_frame_data[5] = (unsigned char)((address & 0xFF000000) >> 24);	// MSB (Byte 3) of Address 		//ex:00
    hid_frame_data[6] = (unsigned char) (page_count & 0x00FF);			// LSB (Byte 0) of Page_Count	//ex:01
    hid_frame_data[7] = (unsigned char)((page_count & 0xFF00) >>  8);	// MSB (Byte 1) of Page_Count 	//ex:00

    // Write frame data to touch
    err = __hidraw_write(hid_frame_data, sizeof(hid_frame_data), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to write frame data, err=0x%x.\r\n", err);
        goto SEND_ERASE_FLASH_SECTION_COMMMAND_EXIT;
    }

    // Success
    err = TP_SUCCESS;

SEND_ERASE_FLASH_SECTION_COMMMAND_EXIT:
    return err;
}

int receive_erase_flash_section_response(void)
{
    int err = TP_SUCCESS;
    unsigned char erase_flash_section_response_data[2] = {0};

    // Read Erase Flash Section Response
    err = read_data(erase_flash_section_response_data, sizeof(erase_flash_section_response_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if(err != TP_SUCCESS) // Error or Timeout
    {
        ERROR_PRINTF("Fail to receive Erase Flash Section Response data! err=0x%x.\r\n", err);
        goto RECEIVE_ERASE_FLASH_SECTION_RESPONSE_EXIT;
    }
    DEBUG_PRINTF("Erase Flash Section Response: 0x%02x, 0x%02x.\r\n", erase_flash_section_response_data[0], erase_flash_section_response_data[1]);

    /* Check if Correct Response */
    if((erase_flash_section_response_data[0] != 0xAA) || (erase_flash_section_response_data[1] != 0xAA))
    {
        ERROR_PRINTF("Unknown Response: %x %x.\n", erase_flash_section_response_data[0], erase_flash_section_response_data[1]);
        err = TP_ERR_DATA_PATTERN;
        goto RECEIVE_ERASE_FLASH_SECTION_RESPONSE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

RECEIVE_ERASE_FLASH_SECTION_RESPONSE_EXIT:
    return err;
}

