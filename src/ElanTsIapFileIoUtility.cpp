/** @file

  Implementation of Function Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsIapFileIoUtility.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>     /* close */
#include <sys/stat.h>
#include <sys/types.h>
#include "ErrCode.h"
#include "ElanTsIapFileIoUtility.h"

/***************************************************
 * Function Implements
 ***************************************************/

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

// Remark ID
int get_remark_id_from_firmware(unsigned short *p_remark_id)
{
	int err = TP_SUCCESS,
        read_byte = 0;
    unsigned char data[2] = {0};
    unsigned short remark_id = 0;

	// Check if Parameter Invalid
    if (p_remark_id == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_remark_id=0x%p)\r\n", __func__, p_remark_id);
        err = TP_ERR_INVALID_PARAM;
        goto GET_REMARK_ID_FROM_FW_EXIT;
    }

    // Reposition fd to position '-4' from the end of file.
    lseek(g_firmware_fd, -4L, SEEK_END);

    // Read Data from File   
	read_byte = read(g_firmware_fd, data, 2);  // 15 63 XX XX
	if(read_byte != 2)
	{
		ERROR_PRINTF("%s: Fail to get 2 bytes of remark_id from fd %d! (result=%d, errno=%d)\r\n", __func__, g_firmware_fd, err, errno);
		err = TP_GET_DATA_FAIL;
        goto GET_REMARK_ID_FROM_FW_EXIT;
	}
    
    // Read FW Remark ID
	remark_id = data[0] + (data[1] << 8);
    DEBUG_PRINTF("Remark ID: %04x.\r\n", remark_id);

    *p_remark_id = remark_id;
    err = TP_SUCCESS;

GET_REMARK_ID_FROM_FW_EXIT:
	return err;
}
