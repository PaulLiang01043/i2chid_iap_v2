/** @file

  Implementation of Firmware File I/O Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2019, All Rights Reserved

  Module Name:
	ElanTsFwFileIoUtility.cpp

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
#include "ElanTsFwFileIoUtility.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

// Firmware File Information
int  g_firmware_fd = -1;

/***************************************************
 * Function Implements
 ***************************************************/

// Open FW file and store file handler in global variable.
int open_firmware_file(char *filename, size_t filename_len)
{
    int err = TP_SUCCESS,
        fd = 0;

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

    // Make Sure File Not Been Opened
    if(g_firmware_fd >= 0)
    {
        ERROR_PRINTF("%s: File \'%s\' has been opened. fd=%d.\r\n", __func__, filename, g_firmware_fd);
        err = EBUSY;
        goto OPEN_FIRMWARE_FILE_EXIT;
    }

    // Open File
    DEBUG_PRINTF("Open file \"%s\".\r\n", filename);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        ERROR_PRINTF("%s: Failed to open firmware file \'%s\', errno=%d.\r\n", __func__, filename, errno);
        err = TP_ERR_FILE_NOT_FOUND;
        goto OPEN_FIRMWARE_FILE_EXIT;
    }

    // Reset Read/Write Position of File Handler
    lseek(fd, 0, SEEK_SET);

    DEBUG_PRINTF("File \"%s\" opened, fd=%d.\r\n", filename, fd);
    g_firmware_fd = fd;

    // Success
    err = TP_SUCCESS;

OPEN_FIRMWARE_FILE_EXIT:
    return err;
}

// Close FW file with file handler stored in global variable.
int close_firmware_file(void)
{
    int err = TP_SUCCESS;

    if(g_firmware_fd >= 0)
    {
        // Close File
        err = close(g_firmware_fd);
        if(err < 0)
        {
            ERROR_PRINTF("Failed to close firmware file(fd=%d), errno=%d.\r\n", g_firmware_fd, errno);
            err = TP_ERR_IO_ERROR;
            goto CLOSE_FIRMWARE_FILE_EXIT;
        }

        // Reset File Descriptor
        g_firmware_fd = -1;
    }

CLOSE_FIRMWARE_FILE_EXIT:
    return err;
}

int get_firmware_size(int *firmware_size)
{
    int err = TP_SUCCESS;
    struct stat file_stat;

    // Make Sure File Handler is Valid
    if(g_firmware_fd < 0)
    {
        ERROR_PRINTF("%s: FW file has not been opened. firmware_fd=%d.\r\n", __func__, g_firmware_fd);
        err = EBADFD;
        goto GET_FIRMWARE_SIZE_EXIT;
    }

    // Get File Status
    err = fstat(g_firmware_fd, &file_stat);
    if(err < 0)
    {
        ERROR_PRINTF("%s: Fail to Get Firmware File Size! errno=%d.\r\n", __func__, errno);
        err = TP_ERR_FILE_NOT_FOUND;
    }
    else
    {
        //DEBUG_PRINTF("%s: File Size = %zd.\r\n", __func__, file_stat.st_size);
        *firmware_size = file_stat.st_size;
        err = TP_SUCCESS;
    }

GET_FIRMWARE_SIZE_EXIT:
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
    int err = TP_SUCCESS,
        read_byte = 0;

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
    read_byte = read(g_firmware_fd, data, data_size);
    if(read_byte != data_size)
    {
        ERROR_PRINTF("%s: Fail to get %d bytes from fd %d! (read_byte=%d, errno=%d)\r\n", __func__, data_size, g_firmware_fd, read_byte, errno);
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
    off_t result = 0,
          file_cur_position = 0;

    //
    // Validate Input Parameters
    //

    // Remark ID Buffer
    if (p_remark_id == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_remark_id=0x%p)\r\n", __func__, p_remark_id);
        err = TP_ERR_INVALID_PARAM;
        goto GET_REMARK_ID_FROM_FW_EXIT;
    }

    //
    // Save Current R/W Position of File Handler
    //
    result = lseek(g_firmware_fd, 0, SEEK_CUR);
    if(result < 0)
    {
        ERROR_PRINTF("%s: Fail to Get Current File R/W Position! (result=%ld, errno=%d)\r\n", __func__, result, errno);
        err = TP_ERR_FILE_IO_ERROR;
        goto GET_REMARK_ID_FROM_FW_EXIT;
    }
    file_cur_position = result;
    //DEBUG_PRINTF("%s: Current File R/W Position: %ld.\r\n", __func__, file_cur_position);

    //
    // Get Remark ID from eKT FW File
    //

    // Re-locate File R/W Position to '-4' (the Last 4 Byte) from the End of File.
    lseek(g_firmware_fd, -4L, SEEK_END);

    // Read Data from File
    read_byte = read(g_firmware_fd, data, 2);  // 15 63 XX XX
    if(read_byte != 2)
    {
        err = TP_GET_DATA_FAIL;
        ERROR_PRINTF("%s: Fail to get 2 bytes of remark_id from fd %d! (read_byte=%d, errno=%d)\r\n", __func__, g_firmware_fd, read_byte, errno);
        goto GET_REMARK_ID_FROM_FW_EXIT_1;
    }

    // Read FW Remark ID
    //remark_id = data[0] + (data[1] << 8);
    remark_id = TWO_BYTE_ARRAY_TO_WORD(data);
    DEBUG_PRINTF("%s: Remark ID: %04x.\r\n", __func__, remark_id);

    // Load Remark ID to Input Buffer
    *p_remark_id = remark_id;

    // Success
    err = TP_SUCCESS;

GET_REMARK_ID_FROM_FW_EXIT_1:

    //
    // Restore to Current R/W Position of File Handler
    //
    //DEBUG_PRINTF("%s: Restore File R/W Position to %ld.\r\n", __func__, file_cur_position);
    lseek(g_firmware_fd, file_cur_position, SEEK_SET);

GET_REMARK_ID_FROM_FW_EXIT:

    return err;
}
