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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>     /* close */
#include <sys/stat.h>
#include <sys/types.h>
#include "ErrCode.h"
#include "ElanTsFwFileIoUtility.h"		// Inherit Variables & Functions from ElanTsFwFileIoUtility.h
#include "ElanGen8TsFwFileIoUtility.h"
#include "ElanGen8TsFuncApi.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

// Validate eKTL FW:
// Check Format of eKTL FW
int validate_ektl_fw(bool *p_result)
{
    int err = TP_SUCCESS;
    unsigned int header_length = 0;
    unsigned char header_page[ELAN_EKTL_FW_PAGE_SIZE] = {0};
    char header_title[6] = {0},
         header_end[3] = {0};
    bool is_ektl_fw = false;
    off_t result = 0,
          file_cur_position = 0;

    //
    // Validate Input Parameters
    //

    // Result Buffer
    if(p_result == NULL)
    {
        ERROR_PRINTF("%s: NULL Pointer of Result Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto VALIDATE_EKTL_FW_EXIT;
    }

    //
    // Initilize Data Variables
    //

    // eKTL Header Page
    memset(header_page, 0, sizeof(header_page));

    // eKTL Header Title & End
    memset(header_title, 0, sizeof(header_title));
    memset(header_end, 0, sizeof(header_end));

    //
    // Save Current R/W Position of File Handler
    //
    result = lseek(g_firmware_fd, 0, SEEK_CUR);
    if(result < 0)
    {
        err = TP_ERR_FILE_IO_ERROR;
        ERROR_PRINTF("%s: Fail to Get Current File R/W Position! (errno=%d, err=0x%x)\r\n", __func__, errno, err);
        goto VALIDATE_EKTL_FW_EXIT;
    }
    file_cur_position = result;
    //DEBUG_PRINTF("%s: Current File R/W Position: %ld.\r\n", __func__, file_cur_position);

    //
    // Get Header Page
    //

    // Reset Current R/W Position of File Handler
    lseek(g_firmware_fd, 0, SEEK_SET);

    // Get Header Page (The First Page) Data
    err = retrieve_data_from_firmware(header_page, sizeof(header_page));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Header Page Data! err=0x%x.\r\n", __func__, err);
        goto VALIDATE_EKTL_FW_EXIT_1;
    }

    //
    // Validate Header Page Format
    //

    /*					                     eKTL Header Page Format						                                 *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * | Byte Offset | Field                | Size | Type               | Content                                          | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |        0    | charArrayHeaderTitle |   6  | char array         | {'h', 'e', 'a', 'd', 'e', 'r'}                   | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |        6    | nHeaderLength        |   4  | unsigned int       | 0x000007fe / 2046                                | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       10    | nArrayVerLibHex2Ektl |  16  | unsigned int array | {0x00000001, 0x00000003, 0x00000000, 0x00000000} | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       26    | nEraseSectionCount   |   4  | unsigned int       | 0x00000002                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       30    | nEraseSectionAddres1 |   4  | unsigned int       | 0x0003F800                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       34    | nEraseSectionPages1  |   4  | unsigned int       | 0x00000001                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       38    | nEraseSectionAddres1 |   4  | unsigned int       | 0x00004000                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       42    | nEraseSectionPages1  |   4  | unsigned int       | 0x00000077                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |							           ...							                                               | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |     2053    | charArrayHeaderEnd   |   3  | char array         | {'e', 'o', 'f'}                                  | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     */

    // Validate Header Title => {'h', 'e', 'a', 'd', 'e', 'r'}
    memcpy(header_title, &header_page[0], sizeof(header_title));
    if((header_title[0] != 'h') || \
       (header_title[1] != 'e') || \
       (header_title[2] != 'a') || \
       (header_title[3] != 'd') || \
       (header_title[4] != 'e') || \
       (header_title[5] != 'r'))
    {
        // Patten Mismatched
        DEBUG_PRINTF("%s: Invalid Header Title! {\'%c\', \'%c\', \'%c\', \'%c\', \'%c\', \'%c\'}.\r\n", \
                     __func__, header_title[0], header_title[1], header_title[2], header_title[3], \
                     header_title[4], header_title[5]);
        is_ektl_fw = false;
        goto VALIDATE_EKTL_FW_EXIT_2;
    }

    // Validate Header End => {'e', 'o', 'f'}
    memcpy(header_end, &header_page[ELAN_EKTL_FW_PAGE_SIZE - sizeof(header_end)], sizeof(header_end));
    if((header_end[0] != 'e') || \
       (header_end[1] != 'o') || \
       (header_end[2] != 'f'))
    {
        // Patten Mismatched
        DEBUG_PRINTF("%s: Invalid Header End! {\'%c\', \'%c\', \'%c\'}.\r\n", \
                     __func__, header_end[0], header_end[1], header_end[2]);
        is_ektl_fw = false;
        goto VALIDATE_EKTL_FW_EXIT_2;
    }

    // Validate Header Length
    header_length = FOUR_BYTE_ARRAY_TO_UINT(&header_page[6]);
    DEBUG_PRINTF("%s: header_length = %d.\r\n", __func__, header_length);
    if(header_length != (ELAN_EKTL_FW_PAGE_SIZE - 6 /* sizeof(charArrayHeaderTitle) */) - 4 /* sizeof(nHeaderLength) */)
    {
        // Patten Mismatched
        DEBUG_PRINTF("%s: Invalid Header Length (%d)!\r\n", __func__, header_length);
        is_ektl_fw = false;
        goto VALIDATE_EKTL_FW_EXIT_2;
    }

    // Patten Matched
    is_ektl_fw = true;

VALIDATE_EKTL_FW_EXIT_2:

    // Load Result to Input Buffer
    *p_result = is_ektl_fw;

    // Success
    err = TP_SUCCESS;

VALIDATE_EKTL_FW_EXIT_1:

    //
    // Restore to Current R/W Position of File Handler
    //
    //DEBUG_PRINTF("%s: Restore File R/W Position to %ld.\r\n", __func__, file_cur_position);
    lseek(g_firmware_fd, file_cur_position, SEEK_SET);

VALIDATE_EKTL_FW_EXIT:
    return err;
}

// Get eKTL Erase Script
// Get Header Page from eKTL FW File & Get An Erase Script Prepared
int get_ektl_erase_script(struct erase_script *p_erase_script, size_t erase_script_size)
{
    int err = TP_SUCCESS;
    unsigned int header_length = 0,
                 array_element_index = 0,
                 erase_section_index = 0;
    unsigned char header_page[ELAN_EKTL_FW_PAGE_SIZE] = {0};
    char header_title[6] = {0},
         header_end[3] = {0};
    struct erase_script EraseScript;

    //
    // Validate Input Parameters
    //

    // Erase Script
    if(p_erase_script == NULL)
    {
        ERROR_PRINTF("%s: NULL Erase Script Pointer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GET_EKTL_ERASE_SCRIPT_EXIT;
    }

    // Erase Script Size
    if(erase_script_size == 0)
    {
        ERROR_PRINTF("%s: Erase Script Size is Zero!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GET_EKTL_ERASE_SCRIPT_EXIT;
    }

    //
    // Initilize Data Variables
    //

    // Erase Script
    memset(&EraseScript, 0, sizeof(struct erase_script));

    // eKTL Header Page
    memset(header_page, 0, sizeof(header_page));

    // eKTL Header Title & End
    memset(header_title, 0, sizeof(header_title));
    memset(header_end, 0, sizeof(header_end));

    //
    // Get Header Page
    //

    // Reset Read/Write Position of File Handler
    lseek(g_firmware_fd, 0, SEEK_SET);

    // Get Header Page (The First Page) Data
    err = retrieve_data_from_firmware(header_page, sizeof(header_page));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Header Page Data! err=0x%x.\r\n", __func__, err);
        goto GET_EKTL_ERASE_SCRIPT_EXIT;
    }

    //
    // Validate Header Page Format
    //

    /*					                     eKTL Header Page Format						                                 *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * | Byte Offset | Field                | Size | Type               | Content                                          | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |        0    | charArrayHeaderTitle |   6  | char array         | {'h', 'e', 'a', 'd', 'e', 'r'}                   | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |        6    | nHeaderLength        |   4  | unsigned int       | 0x000007fe / 2046                                | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       10    | nArrayVerLibHex2Ektl |  16  | unsigned int array | {0x00000001, 0x00000003, 0x00000000, 0x00000000} | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       26    | nEraseSectionCount   |   4  | unsigned int       | 0x00000002                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       30    | nEraseSectionAddres1 |   4  | unsigned int       | 0x0003F800                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       34    | nEraseSectionPages1  |   4  | unsigned int       | 0x00000001                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       38    | nEraseSectionAddres1 |   4  | unsigned int       | 0x00004000                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |       42    | nEraseSectionPages1  |   4  | unsigned int       | 0x00000077                                       | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |							           ...							                                               | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     * |     2053    | charArrayHeaderEnd   |   3  | char array         | {'e', 'o', 'f'}                                  | *
     * +-------------+----------------------+------+--------------------+--------------------------------------------------+ *
     */

    // Validate Header Title => {'h', 'e', 'a', 'd', 'e', 'r'}
    memcpy(header_title, &header_page[0], sizeof(header_title));
    if((header_title[0] != 'h') || \
       (header_title[1] != 'e') || \
       (header_title[2] != 'a') || \
       (header_title[3] != 'd') || \
       (header_title[4] != 'e') || \
       (header_title[5] != 'r'))
    {
        err = TP_ERR_DATA_PATTERN;
        ERROR_PRINTF("%s: Invalid Header Title! {\'%c\', \'%c\', \'%c\', \'%c\', \'%c\', \'%c\'}, err=0x%x.\r\n", \
                     __func__, header_title[0], header_title[1], header_title[2], header_title[3], \
                     header_title[4], header_title[5], err);
        goto GET_EKTL_ERASE_SCRIPT_EXIT;
    }

    // Validate Header End => {'e', 'o', 'f'}
    memcpy(header_end, &header_page[ELAN_EKTL_FW_PAGE_SIZE - sizeof(header_end)], sizeof(header_end));
    if((header_end[0] != 'e') || \
       (header_end[1] != 'o') || \
       (header_end[2] != 'f'))
    {
        err = TP_ERR_DATA_PATTERN;
        ERROR_PRINTF("%s: Invalid Header End! {\'%c\', \'%c\', \'%c\'}, err=0x%x.\r\n", \
                     __func__, header_end[0], header_end[1], header_end[2], err);
        goto GET_EKTL_ERASE_SCRIPT_EXIT;
    }

    // Validate Header Length
    header_length = FOUR_BYTE_ARRAY_TO_UINT(&header_page[6]);
    DEBUG_PRINTF("%s: header_length = %d.\r\n", __func__, header_length);
    if(header_length != (ELAN_EKTL_FW_PAGE_SIZE - 6 /* sizeof(charArrayHeaderTitle) */) - 4 /* sizeof(nHeaderLength) */)
    {
        err = TP_ERR_DATA_PATTERN;
        ERROR_PRINTF("%s: Invalid Header Length (%d)! err=0x%x.\r\n", __func__, header_length, err);
        goto GET_EKTL_ERASE_SCRIPT_EXIT;
    }

    //
    // Parse Header Page
    //

    // libHex2Ektl Version
    for(array_element_index = 0; array_element_index < 4; array_element_index++)
    {
        EraseScript.nArrayVerLibHex2Ektl[array_element_index] = FOUR_BYTE_ARRAY_TO_UINT(&header_page[ 10 + (array_element_index * 4)]);
    }
    DEBUG_PRINTF("%s: libHex2Ektl Version = \"%d.%d.%d.%d\".\r\n", __func__, \
                 EraseScript.nArrayVerLibHex2Ektl[0], EraseScript.nArrayVerLibHex2Ektl[1], \
                 EraseScript.nArrayVerLibHex2Ektl[2], EraseScript.nArrayVerLibHex2Ektl[3]);

    // Erase Section Count
    EraseScript.nEraseSectionCount = FOUR_BYTE_ARRAY_TO_UINT(&header_page[26]);
    DEBUG_PRINTF("%s: Erase Section Count = %d.\r\n", __func__, EraseScript.nEraseSectionCount);

    // Erase Section Setting
    for(erase_section_index = 0; erase_section_index < EraseScript.nEraseSectionCount; erase_section_index++)
    {
        // Address of Erase_Section[Index]
        EraseScript.EraseSection[erase_section_index].address = FOUR_BYTE_ARRAY_TO_UINT(&header_page[ 30 + (erase_section_index * 8)]);

        // Page Count of Erase_Section[Index]
        EraseScript.EraseSection[erase_section_index].page_count = FOUR_BYTE_ARRAY_TO_UINT(&header_page[ 34 + (erase_section_index * 8)]);

        DEBUG_PRINTF("%s: Erase_Section[%d]: Address=0x%08x, Page_Count=%d.\r\n", __func__, \
                     erase_section_index, EraseScript.EraseSection[erase_section_index].address, \
                     EraseScript.EraseSection[erase_section_index].page_count);
    }

    // Load Local Erase Script Instance to Input Buffer
    memcpy(p_erase_script, &EraseScript, sizeof(struct erase_script));

    // Success
    err = TP_SUCCESS;

GET_EKTL_ERASE_SCRIPT_EXIT:
    return err;
}

// Get eKTL FW Page Count
int compute_ektl_fw_page_number(int firmware_size)
{
    /*            eKTL FW File            *
     * +--------------------------------+ *
     * |	Header Page    / 2056 Bytes | *
     * +--------------------------------+ *
     * |	FW Page 1      / 2056 Bytes | *
     * +--------------------------------+ *
     * |	FW Page 2      / 2056 Bytes | *
     * +--------------------------------+ *
     * |			...					| *
     * +--------------------------------+ *
     * |	FW Page (n-1)  / 2056 Bytes | *
     * +--------------------------------+ *
     * |	FW Page n      / 2056 Bytes | *
     * +--------------------------------+ *
     */

    // Return FW Page Count, NOT including Header Page
    return ((firmware_size / ELAN_EKTL_FW_PAGE_SIZE) + ((firmware_size % ELAN_EKTL_FW_PAGE_SIZE) != 0) - 1 /* Header Page */);
}

int compute_ektl_page_number(int firmware_size)
{
    // Return Page Count, including Header Page
    return ((firmware_size / ELAN_EKTL_FW_PAGE_SIZE) + ((firmware_size % ELAN_EKTL_FW_PAGE_SIZE) != 0));
}

// eKTL Page Data
int get_page_data_from_ektl_firmware(unsigned char page_index, unsigned char *p_ektl_page_buf, size_t ektl_page_buf_size)
{
    int err = TP_SUCCESS,
        read_byte = 0;
    unsigned char ektl_fw_page_data[ELAN_EKTL_FW_PAGE_SIZE] = {0};
    off_t result = 0,
          file_cur_position = 0,
          ektl_page_position = 0;

    //
    // Validate Input Parameters
    //

    // eKTL FW Page Buffer & Buffer Size
    if((p_ektl_page_buf == NULL) || (ektl_page_buf_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Input Parameter! (p_ektl_page_buf=0x%p, ektl_page_buf_size=%ld)\r\n", \
                     __func__, p_ektl_page_buf, ektl_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_PAGE_DATA_FROM_EKTL_FW_EXIT;
    }

    //
    // Save Current R/W Position of File Handler
    //
    result = lseek(g_firmware_fd, 0, SEEK_CUR);
    if(result < 0)
    {
        ERROR_PRINTF("%s: Fail to Get Current File R/W Position! (result=%ld, errno=%d)\r\n", __func__, result, errno);
        err = TP_ERR_FILE_IO_ERROR;
        goto GET_PAGE_DATA_FROM_EKTL_FW_EXIT;
    }
    file_cur_position = result;
    //DEBUG_PRINTF("%s: Current File R/W Position: 0x%lx.\r\n", __func__, file_cur_position);

    //
    // Get FW Page Data from eKTL FW File
    //

    // Re-locate File R/W Position to ($(Page_Index) * ELAN_EKTL_FW_PAGE_SIZE) from the Beginning of File.
    ektl_page_position = page_index * ELAN_EKTL_FW_PAGE_SIZE;
    lseek(g_firmware_fd, ektl_page_position, SEEK_SET);

    // Read $(ELAN_EKTL_FW_PAGE_SIZE)-byte Page Data from eKTL File
    read_byte = read(g_firmware_fd, ektl_fw_page_data, sizeof(ektl_fw_page_data));
    if(read_byte != ELAN_EKTL_FW_PAGE_SIZE)
    {
        err = TP_GET_DATA_FAIL;
        ERROR_PRINTF("%s: Fail to get %d bytes of remark_id from fd %d! (read_byte=%d, errno=%d)\r\n", \
                     __func__, ELAN_EKTL_FW_PAGE_SIZE, g_firmware_fd, read_byte, errno);
        goto GET_PAGE_DATA_FROM_EKTL_FW_EXIT_1;
    }

    DEBUG_PRINTF("%s: eKTL FW Page %d from 0x%lx: %02x %02x %02x %02x %02x %02x %02x %02x.\r\n", \
                 __func__, page_index, ektl_page_position, \
                 ektl_fw_page_data[0],  ektl_fw_page_data[1],  ektl_fw_page_data[2],  ektl_fw_page_data[3],  \
                 ektl_fw_page_data[4],  ektl_fw_page_data[5],  ektl_fw_page_data[6],  ektl_fw_page_data[7]);

    // Load Remark ID to Input Buffer
    memcpy(p_ektl_page_buf, ektl_fw_page_data, sizeof(ektl_fw_page_data));

    // Success
    err = TP_SUCCESS;

GET_PAGE_DATA_FROM_EKTL_FW_EXIT_1:

    //
    // Restore to Current R/W Position of File Handler
    //
    //DEBUG_PRINTF("%s: Restore File R/W Position to 0x%lx.\r\n", __func__, file_cur_position);
    lseek(g_firmware_fd, file_cur_position, SEEK_SET);

GET_PAGE_DATA_FROM_EKTL_FW_EXIT:
    return err;
}

// Remark ID
int get_remark_id_from_ektl_firmware(unsigned char *p_gen8_remark_id_buf, size_t gen8_remark_id_buf_size)
{
    int err = TP_SUCCESS,
        firmware_size = 0;
    unsigned int rom_address = 0,
                 ektl_page_count = 0;
    unsigned short ektl_page_data_index = 0;
    unsigned char ektl_page_data[ELAN_EKTL_FW_PAGE_SIZE] = {0},
            gen8_remark_id_data[ELAN_GEN8_REMARK_ID_LEN] = {0};
    bool last_page_is_info_page = false;

    //
    // Validate Input Parameters
    //

    // Remark ID Buffer & Buffer Size
    if((p_gen8_remark_id_buf == NULL) || (gen8_remark_id_buf_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Input Parameter! (p_gen8_remark_id_buf=0x%p, gen8_remark_id_buf_size=%ld)\r\n", \
                     __func__, p_gen8_remark_id_buf, gen8_remark_id_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_REMARK_ID_FROM_EKTL_FW_EXIT;
    }

    //
    // Get the Last FW Page Data
    //

    // Get FW Size
    err = get_firmware_size(&firmware_size);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Firmware Size! err=0x%x.\r\n", __func__, err);
        goto GET_REMARK_ID_FROM_EKTL_FW_EXIT;
    }

    // Get eKTL Page Count (Including Header Page!)
    ektl_page_count = compute_ektl_page_number(firmware_size);

    // Get the Last eKTL Page
    err = get_page_data_from_ektl_firmware((ektl_page_count - 1), ektl_page_data, sizeof(ektl_page_data));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get the Last eKTL Page! err=0x%x.\r\n", __func__, err);
        goto GET_REMARK_ID_FROM_EKTL_FW_EXIT;
    }

    // Validate the Last eKTL Page Data
    rom_address = FOUR_BYTE_ARRAY_TO_UINT(&ektl_page_data[0]);
    if(rom_address == ELAN_GEN8_INFO_ROM_MEMORY_ADDR)
    {
        DEBUG_PRINTF("%s: The Last eKTL Page is Information Page!\r\n", __func__);
        last_page_is_info_page = true;
    }

    // If Last Page is Inforamtion Page, Get the Second to Last eKTL Page
    if(last_page_is_info_page == true)
    {
        DEBUG_PRINTF("%s: Get the Second to Last eKTL Page!\r\n", __func__);
        memset(ektl_page_data, 0, sizeof(ektl_page_data));
        err = get_page_data_from_ektl_firmware((ektl_page_count - 2), ektl_page_data, sizeof(ektl_page_data));
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get the Second to Last eKTL Page! err=0x%x.\r\n", __func__, err);
            goto GET_REMARK_ID_FROM_EKTL_FW_EXIT;
        }
    }

    //
    // Get Remark ID from the Last eKTL FW Page
    //

    /*                        Last eKTL Page Data                          *
     * +----------------+------------------------------------------------+ *
     * | 4-byte Address |                                                | *
     * +----------------+                                                | *
     * |                                                                 | *
     * |                           Page Data                             | *
     * |                                                                 | *
     * |                                                                 | *
     * +-----------------------+-----------------------+-----------------+ *
     * |   16-byte Remark ID   |      16-byte Data     | 4-byte Checksum | *
     * +-----------------------+-----------------------+-----------------+ *
     */

    // Re-locate Data Position to '-36' (the Last 36 Byte) from the End of File.
    ektl_page_data_index = ELAN_EKTL_FW_PAGE_SIZE - 4 /* checksum */ - 16 /* data */ - 16 /* Remark ID Data */;

    // Get $(ELAN_GEN8_REMARK_ID_LEN)-byte Remark ID Data
    memcpy(gen8_remark_id_data, &ektl_page_data[ektl_page_data_index], ELAN_GEN8_REMARK_ID_LEN);

    DEBUG_PRINTF("%s: Gen8 Remark ID from Last eKTL FW Page Data[%d]: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x.\r\n", \
                 __func__, ektl_page_data_index, \
                 gen8_remark_id_data[0],  gen8_remark_id_data[1],  gen8_remark_id_data[2],  gen8_remark_id_data[3],  \
                 gen8_remark_id_data[4],  gen8_remark_id_data[5],  gen8_remark_id_data[6],  gen8_remark_id_data[7],  \
                 gen8_remark_id_data[8],  gen8_remark_id_data[9],  gen8_remark_id_data[10], gen8_remark_id_data[11], \
                 gen8_remark_id_data[12], gen8_remark_id_data[13], gen8_remark_id_data[14], gen8_remark_id_data[15]);

    // Load Remark ID to Input Buffer
    memcpy(p_gen8_remark_id_buf, gen8_remark_id_data, sizeof(gen8_remark_id_data));

    // Success
    err = TP_SUCCESS;

GET_REMARK_ID_FROM_EKTL_FW_EXIT:
    return err;
}

