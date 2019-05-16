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
#include "ElanTsFuncUtility.h"

/***************************************************
 * TP Functions
 ***************************************************/

// Power Status
int send_set_power_status_command(int mode)
{
    int err = TP_SUCCESS;
    unsigned char set_pwr_status_cmd[4] = {0x54, 0x50, 0x00, 0x01};

    set_pwr_status_cmd[1] |= mode;

    /* Send Set Pwr Status Command */
    DEBUG_PRINTF("cmd: 0x%02x, 0x%02x, 0x%02x, 0x%02x.\r\n", set_pwr_status_cmd[0], set_pwr_status_cmd[1], \
                 set_pwr_status_cmd[2], set_pwr_status_cmd[3]);
    err = write_cmd(set_pwr_status_cmd, 4, ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Set Power Status command! errno=0x%x.\r\n", err);

    return err;
}

// FW ID
int send_fw_id_command(void)
{
	int err = TP_SUCCESS;
    unsigned char fw_id_cmd[4] = {0x53, 0xf0, 0x00, 0x01};


    /* Send FW ID Command to touch */
    DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", fw_id_cmd[0], fw_id_cmd[1], fw_id_cmd[2], fw_id_cmd[3]);
    err = write_cmd(fw_id_cmd, sizeof(fw_id_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send FW ID command! errno=0x%x.\r\n", err);

    return err;
}

int read_fw_id_data(void)
{
    int err = TP_SUCCESS,
		major_fw_id = 0,
        minor_fw_id = 0;
    unsigned char cmd_data[4] = {0};

    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to read FW ID data, errno=0x%x.\n", err);
        goto READ_FW_ID_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

    /* Check if Data is Firmware ID */
    if ((cmd_data[0] == 0x52) && (((cmd_data[1] & 0xf0) >> 4) == 0xf))
    {
        major_fw_id = ((cmd_data[1] & 0x0f) << 4) | ((cmd_data[2] & 0xf0) >> 4);
        minor_fw_id = ((cmd_data[2] & 0x0f) << 4) | ((cmd_data[3] & 0xf0) >> 4);
        printf("Firmware ID: %02x.%02x\r\n", major_fw_id, minor_fw_id);
    }

    err = TP_SUCCESS;

READ_FW_ID_DATA_EXIT:
    return err;
}

// FW Version
int send_fw_version_command(void)
{
    int err = TP_SUCCESS;
    unsigned char fw_ver_cmd[4] = {0x53, 0x00, 0x00, 0x01};

    /* Send FW ID Command to touch */
    DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", fw_ver_cmd[0], fw_ver_cmd[1], fw_ver_cmd[2], fw_ver_cmd[3]);
    err = write_cmd(fw_ver_cmd, sizeof(fw_ver_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send FW Version command! errno=0x%x.\r\n", err);

    return err;
}

int read_fw_version_data(void)
{
    int major_fw_ver = 0,
        minor_fw_ver = 0,
        err = TP_SUCCESS;
    unsigned char cmd_data[4] = {0};

    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to read FW Version data, errno=0x%x.\r\n", err);
        goto READ_FW_VERSION_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

    /* Check if Data is Firmware Version */
    if ((cmd_data[0] == 0x52) && (((cmd_data[1] & 0xf0) >> 4) == 0))
    {
        major_fw_ver = ((cmd_data[1] & 0x0f) << 4) | ((cmd_data[2] & 0xf0) >> 4);
        minor_fw_ver = ((cmd_data[2] & 0x0f) << 4) | ((cmd_data[3] & 0xf0) >> 4);
        printf("Firmware Version: %02x.%02x\r\n", major_fw_ver, minor_fw_ver);
    }

    err = TP_SUCCESS;

READ_FW_VERSION_DATA_EXIT:
    return err;
}

int send_test_version_command(void)
{
    int err = TP_SUCCESS;
    unsigned char test_ver_cmd[4] = {0x53, 0xe0, 0x00, 0x01};

    /* Send Test Version Command to touch */
    DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", test_ver_cmd[0], test_ver_cmd[1], test_ver_cmd[2], test_ver_cmd[3]);
    err = write_cmd(test_ver_cmd, sizeof(test_ver_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Test Version command! errno=0x%x.\r\n", err);

    return err;
}

// Test Version
int read_test_version_data(void)
{
    int err = TP_SUCCESS,
		test_ver = 0,
        solution_ver = 0;
    unsigned char cmd_data[4] = {0};

    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to receive Test Version data, errno=0x%x.\r\n", err);
        goto READ_TEST_VERSION_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

    /* Check if Data is for Test Version */
    if ((cmd_data[0] == 0x52) && (((cmd_data[1] & 0xf0) >> 4) == 0xe))
    {
        test_ver = ((cmd_data[1] & 0x0f) << 4) | ((cmd_data[2] & 0xf0) >> 4);
        solution_ver = ((cmd_data[2] & 0x0f) << 4) | ((cmd_data[3] & 0xf0) >> 4);
        printf("Test Version: %02x.%02x\r\n", test_ver, solution_ver);
    }

    err = TP_SUCCESS;

READ_TEST_VERSION_DATA_EXIT:
    return err;
}

// Boot Code Version
int send_boot_code_version_command(void)
{
    int err = TP_SUCCESS;
    unsigned char bc_ver_cmd[4] = {0x53, 0x10, 0x00, 0x01};

    /* Send Boot Code Version Command to touch */
    DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", bc_ver_cmd[0], bc_ver_cmd[1], bc_ver_cmd[2], bc_ver_cmd[3]);
    err = write_cmd(bc_ver_cmd, sizeof(bc_ver_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Boot Code Version command! errno=0x%x.\r\n", err);

    return err;
}

int read_boot_code_version_data(void)
{
    int err = TP_SUCCESS,
		major_bc_ver = 0,
        minor_bc_ver = 0;
    unsigned char cmd_data[4] = {0};

    err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_DATA_TIMEOUT_MSEC);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to read Boot Code Version data, errno=0x%x.\r\n", err);
        goto READ_BOOT_CODE_VERSION_DATA_EXIT;
    }
    DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

    /* Check if Data is Boot Code Version */
    if ((cmd_data[0] == 0x52) && (((cmd_data[1] & 0xf0) >> 4) == 0x1))
    {
        major_bc_ver = ((cmd_data[1] & 0x0f) << 4) | ((cmd_data[2] & 0xf0) >> 4);
        minor_bc_ver = ((cmd_data[2] & 0x0f) << 4) | ((cmd_data[3] & 0xf0) >> 4);
        printf("Boot Code Version: %02x.%02x\r\n", major_bc_ver, minor_bc_ver);
    }

    err = TP_SUCCESS;

READ_BOOT_CODE_VERSION_DATA_EXIT:
    return err;
}

// Calibration
int send_rek_command(void)
{ 
	int err = TP_SUCCESS;
	unsigned char write_flash_key_cmd[4]	= {0x54, 0xc0, 0xe1, 0x5a},
				  rek_cmd[4]				= {0x54, 0x29, 0x00, 0x01};  
   
	/* Send Write Flash Key Command */
	DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", write_flash_key_cmd[0], write_flash_key_cmd[1], write_flash_key_cmd[2], write_flash_key_cmd[3]);
    err = write_cmd(write_flash_key_cmd, sizeof(write_flash_key_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
	{
        ERROR_PRINTF("Fail to send Write Flash Key command! errno=0x%x.\r\n", err);
		goto SEND_REK_COMMAND_EXIT;
	}

	/* Send Re-Calibration Command */
	DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", rek_cmd[0], rek_cmd[1], rek_cmd[2], rek_cmd[3]);
    err = write_cmd(rek_cmd, sizeof(rek_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
	{
        ERROR_PRINTF("Fail to send Re-Calibration command! errno=0x%x.\r\n", err);
		goto SEND_REK_COMMAND_EXIT;
	}

	err = TP_SUCCESS;

SEND_REK_COMMAND_EXIT:
	return err;
}

int receive_rek_response(void)
{
	int err = TP_SUCCESS; 
	unsigned char cmd_data[4] = {0};

	err = read_data(cmd_data, sizeof(cmd_data), ELAN_READ_CALI_RESP_TIMEOUT_MSEC);
	if(err != TP_SUCCESS) // Error or Timeout
	{
		ERROR_PRINTF("Re-Calibration failed! errno=%d.\r\n", err);
        goto RECEIVE_REK_RESPONSE_EXIT;
	}
	DEBUG_PRINTF("cmd_data: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", cmd_data[0], cmd_data[1], cmd_data[2], cmd_data[3]);

	/* Check if Correct Response */
	if((cmd_data[0]==0x66) && (cmd_data[1]==0x66) && (cmd_data[2]==0x66) && (cmd_data[3]==0x66)) // Calibrated
	{
		printf("Re-Calibration success.\r\n");
	}

	err = TP_SUCCESS;

RECEIVE_REK_RESPONSE_EXIT:
	return err;
}

// Test Mode
int send_enter_test_mode_command(void)
{
	int err = TP_SUCCESS;
    unsigned char enter_test_mode_cmd[4] = {0x55, 0x55, 0x55, 0x55};

    /* Send Enter Test Mode Command */
    DEBUG_PRINTF("cmd: 0x%02x, 0x%02x, 0x%02x, 0x%02x.\r\n", enter_test_mode_cmd[0], enter_test_mode_cmd[1], \
                 enter_test_mode_cmd[2], enter_test_mode_cmd[3]);
    err = write_cmd(enter_test_mode_cmd, 4, ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Enter Test Mode command! errno=0x%x.\r\n", err);

    return err;
}

int send_exit_test_mode_command(void)
{
	int err = TP_SUCCESS;
    unsigned char exit_test_mode_cmd[4] = {(unsigned char)0xa5, (unsigned char)0xa5, (unsigned char)0xa5, (unsigned char)0xa5};

    /* Send Exit Test Mode Command */
    DEBUG_PRINTF("cmd: 0x%02x, 0x%02x, 0x%02x, 0x%02x.\r\n", exit_test_mode_cmd[0], exit_test_mode_cmd[1], \
                 exit_test_mode_cmd[2], exit_test_mode_cmd[3]);
    err = write_cmd(exit_test_mode_cmd, 4, ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Enter Test Mode command! errno=0x%x.\r\n", err);

    return err;
}

// Bulk ROM Data
int send_show_bulk_rom_data_command(unsigned short addr, unsigned short len)
{
	int err = TP_SUCCESS;
    unsigned char show_bulk_rom_data_cmd[6] =  {0x59, 0x10, 0x00, 0x00, 0x00, 0x00}; /* Show Bulk ROM Data Command */

	/* Set Address & Length */
	show_bulk_rom_data_cmd[2] = (addr & 0xFF00) >> 8;	//ADDR_H
	show_bulk_rom_data_cmd[3] =  addr & 0x00FF; 		//ADDR_L
	show_bulk_rom_data_cmd[4] = (len  & 0xFF00) >> 8;	//LEN_H
	show_bulk_rom_data_cmd[5] =  len  & 0x00FF;			//LEN_L

    /* Send Show Bulk ROM Data Command */
    DEBUG_PRINTF("cmd: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x.\r\n", \
					show_bulk_rom_data_cmd[0], show_bulk_rom_data_cmd[1], show_bulk_rom_data_cmd[2], \
					show_bulk_rom_data_cmd[3], show_bulk_rom_data_cmd[4], show_bulk_rom_data_cmd[5]);
    err = write_cmd(show_bulk_rom_data_cmd, 6, ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Show Bulk ROM Data command! errno=0x%x.\r\n", err);

    return err;
}

// IAP Mode
int send_enter_iap_command(void)
{
	int err = TP_SUCCESS;
	unsigned char write_flash_key_cmd[4]	= {0x54, 0xc0, 0xe1, 0x5a},
				  enter_iap_cmd[4]			= {0x54, 0x00, 0x12, 0x34};  
   
	/* Send Write Flash Key Command */
	DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", write_flash_key_cmd[0], write_flash_key_cmd[1], write_flash_key_cmd[2], write_flash_key_cmd[3]);
    err = write_cmd(write_flash_key_cmd, sizeof(write_flash_key_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
	{
        ERROR_PRINTF("Fail to send Write Flash Key command! errno=0x%x.\r\n", err);
		goto SEND_ENTER_IAP_COMMAND_EXIT;
	}

	/* Send Enter IAP Command */
	DEBUG_PRINTF("cmd: 0x%x, 0x%x, 0x%x, 0x%x.\r\n", enter_iap_cmd[0], enter_iap_cmd[1], enter_iap_cmd[2], enter_iap_cmd[3]);
    err = write_cmd(enter_iap_cmd, sizeof(enter_iap_cmd), ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
	{
        ERROR_PRINTF("Fail to send Enter IAP command! errno=0x%x.\r\n", err);
		goto SEND_ENTER_IAP_COMMAND_EXIT;
	}

	// Success
	err = TP_SUCCESS;

SEND_ENTER_IAP_COMMAND_EXIT:
	return err;
}

int send_slave_address(void)
{
	int err = TP_SUCCESS;
    unsigned char slave_addr =(unsigned char)(ELAN_I2C_SLAVE_ADDR >> 1); // 7-Bit I2C Slave Address

    /* Send Show Bulk ROM Data Command */
    DEBUG_PRINTF("cmd: 0x%02x.\r\n", slave_addr);
    err = write_cmd(&slave_addr, 1, ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if (err != TP_SUCCESS)
        ERROR_PRINTF("Fail to send Elan TS I2C Slave Address! errno=0x%x.\r\n", err);

    return err;
}

// Flash Write
int send_flash_write_command(void)
{
   int err = TP_SUCCESS;
   unsigned char write_to_flash_cmd = 0x22; // Vendor Command
   
   /* Send Write to Flash Command (Vendor Command) */
   DEBUG_PRINTF("vendor_cmd: 0x%02x.\r\n", write_to_flash_cmd);
   err = write_vendor_cmd(&write_to_flash_cmd, 1, ELAN_WRITE_DATA_TIMEOUT_MSEC);
   if(err != TP_SUCCESS)
      ERROR_PRINTF("Failed to write vendor command 0x%x, errno=%d.\r\n", write_to_flash_cmd, err);
   
    return err;
}

int receive_flash_write_response(void)
{
	int err = TP_SUCCESS;
	unsigned char flash_write_response_data[2] = {0};

	// Read Flash Write Response
	err = read_data(flash_write_response_data, sizeof(flash_write_response_data), ELAN_READ_DATA_TIMEOUT_MSEC);
	if(err != TP_SUCCESS) // Error or Timeout
	{
		ERROR_PRINTF("Fail to receive Flash Write Response data! errno=%d.\r\n", err);
        goto READ_FLASH_WRITE_RESPONSE_EXIT;
	}
	DEBUG_PRINTF("flash_write_response: 0x%02x, 0x%02x.\r\n", flash_write_response_data[0], flash_write_response_data[1]);

	/* Check if Correct Response */
	if((flash_write_response_data[0] != 0xAA) || (flash_write_response_data[1] != 0xAA))
	{
		ERROR_PRINTF("Unknown Response: %x %x.\n", flash_write_response_data[0], flash_write_response_data[1]);
		err = TP_ERR_DATA_PATTERN;
		goto READ_FLASH_WRITE_RESPONSE_EXIT;
	}

	// Success
	err = TP_SUCCESS;

READ_FLASH_WRITE_RESPONSE_EXIT:
    return err;
}

// Hello Packet
// Bridge CMD 0x18: If command <0x18> is issued, feedback Hello packet for Recovery Mode.
int send_request_hello_packet_command(void)
{
	int err = TP_SUCCESS;
	unsigned char request_iap_recovery_hello_packet_cmd = 0x18; // Vendor Command
   
	/* Send Request IAP Hello Packet Command (Vendor Command) */
	DEBUG_PRINTF("vendor_cmd: 0x%02x.\r\n", request_iap_recovery_hello_packet_cmd);
	err = write_vendor_cmd(&request_iap_recovery_hello_packet_cmd, 1, ELAN_WRITE_DATA_TIMEOUT_MSEC);
	if(err != TP_SUCCESS)
		ERROR_PRINTF("Failed to write vendor command 0x%x, errno=%d.\r\n", request_iap_recovery_hello_packet_cmd, err);
   
    return err;
}

