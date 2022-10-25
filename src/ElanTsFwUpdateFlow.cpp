/** @file

  Header of APIs of Firmware Update Flow for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsFwUpdateFlow.cpp

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#include "InterfaceGet.h"
#include "ElanTsFuncApi.h"
#include "ElanTsFwFileIoUtility.h"
#include "ElanTsFwUpdateFlow.h"
#include "ElanGen8TsFwFileIoUtility.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

// Firmware Information
int get_firmware_information(message_mode_t msg_mode)
{
    int err = TP_SUCCESS;
    unsigned short fw_id = 0,
                   fw_version = 0,
                   test_version = 0,
                   bc_version = 0;

    if(msg_mode == SILENT_MODE) // Enable Silent Mode
    {
        // Firmware Version
        err = get_fw_version(&fw_version);
        if(err != TP_SUCCESS)
            goto GET_FW_INFO_EXIT;
        printf("%04x", fw_version);
    }
    else // Normal Mode
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

        // Test Version
        err = get_test_version(&test_version);
        if(err != TP_SUCCESS)
            goto GET_FW_INFO_EXIT;
        /* [Note] 2022/05/03
         * Change Output String of Test Version for FAE's Request.
         */
        //printf("Test Version: %02x.%02x\r\n", HIGH_BYTE(test_version), LOW_BYTE(test_version));
        printf("Test-Solution Version: %02x.%02x\r\n", HIGH_BYTE(test_version), LOW_BYTE(test_version));

        // Boot Code Version
        err = get_boot_code_version(&bc_version);
        if(err != TP_SUCCESS)
            goto GET_FW_INFO_EXIT;
        printf("Boot Code Version: %02x.%02x\r\n", HIGH_BYTE(bc_version), LOW_BYTE(bc_version));
    }

GET_FW_INFO_EXIT:
    return err;
}

// Calibration Counter
int get_calibration_counter(message_mode_t msg_mode)
{
    int err = TP_SUCCESS;
    unsigned short calibration_counter = 0;

    // Calibration Counter
    err = get_rek_counter(&calibration_counter);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Calibration Counter! err=0x%x.\r\n", __func__, err);
        goto GET_CALIBRATION_COUNTER_EXIT;
    }


    switch (msg_mode)
    {
        case FULL_MESSAGE:
            printf("--------------------------------\r\n");
            printf("Calibration Counter: %04x.\r\n", calibration_counter);
            break;

        case SILENT_MODE:
            printf("%04x", calibration_counter);
            break;

        case NO_MESSAGE:
        default:
            // Do Nothing
            break;
    }

    // Success
    err = TP_SUCCESS;

GET_CALIBRATION_COUNTER_EXIT:
    return err;
}

// Remark ID Check
int check_remark_id(bool recovery)
{
    int err = TP_SUCCESS;
    unsigned short remark_id_from_rom = 0,
                   remark_id_from_fw  = 0;

    // Get Remark ID from ROM
    err = get_rom_data(ELAN_INFO_ROM_REMARK_ID_MEMORY_ADDR, recovery, &remark_id_from_rom);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Remark ID from ROM! err=0x%x.\r\n", __func__, err);
        goto CHECK_REMARK_ID_EXIT;
    }

    // Read Remark ID from Firmware
    err = get_remark_id_from_firmware(&remark_id_from_fw);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Remark ID from Firmware! err=0x%x.\r\n", __func__, err);
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

// Firmware Update
int update_firmware(char *filename, size_t filename_len, bool recovery, int skip_action_code)
{
    int err = TP_SUCCESS,
        firmware_size = 0,
        page_count = 0,
        block_count = 0,
        block_index = 0,
        block_page_num = 0;
    unsigned short fw_version = 0,
                   fw_bc_version = 0,
                   bc_bc_version = 0;
    unsigned char hello_packet = 0,
                  info_page_buf[ELAN_FIRMWARE_PAGE_SIZE] = {0},
                  page_block_buf[ELAN_FIRMWARE_PAGE_SIZE * 30] = {0},
                  bc_ver_high_byte = 0,
                  bc_ver_low_byte = 0,
                  iap_version = 0,
                  solution_id = 0;
    bool remark_id_check = false,
         skip_remark_id_check = false,
         skip_information_update = false,
         is_ektl_fw = false;
#if defined(__ENABLE_DEBUG__) && defined(__ENABLE_SYSLOG_DEBUG__)
    bool bDisableOutputBufferDebug = false;
#endif //__ENABLE_SYSLOG_DEBUG__ && __ENABLE_SYSLOG_DEBUG__

    //
    // Validate Arguments
    //

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

    // Check if eKTL FW
    err = validate_ektl_fw(&is_ektl_fw);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Check if eKTL FW! err=0x%x.\r\n", __func__, err);
        goto UPDATE_FIRMWARE_EXIT;
    }
    if(is_ektl_fw == true)
    {
        err = TP_ERR_DATA_PATTERN;
        ERROR_PRINTF("%s: File \"%s\" is eKTL FW! err=0x%x.\r\n", __func__, filename, err);
        goto UPDATE_FIRMWARE_EXIT;
    }

    printf("--------------------------------\r\n");
    printf("FW Path: \"%s\".\r\n", filename);

    //
    // Configure Behavior Settings
    //

    // Set Global Flag of Skip Action Code
    if((skip_action_code & ACTION_CODE_REMARK_ID_CHECK) == ACTION_CODE_REMARK_ID_CHECK)
        skip_remark_id_check = true;
    if((skip_action_code & ACTION_CODE_INFORMATION_UPDATE) == ACTION_CODE_INFORMATION_UPDATE)
        skip_information_update = true;
    DEBUG_PRINTF("skip_remark_id_check: %s, skip_information_update: %s.\r\n", \
                 (skip_remark_id_check) ? "true" : "false", \
                 (skip_information_update) ? "true" : "false");
    //
    // Information Page Update
    //
    if((recovery == false) && (skip_information_update == false)) // Normal Mode & Don't Skip Information (Section) Update
    {
        // FW Version
        err = get_fw_version(&fw_version);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get FW Version! err=0x%x.\r\n", __func__, err);
            goto UPDATE_FIRMWARE_EXIT;
        }
        DEBUG_PRINTF("FW Version: 0x%04x.\r\n", fw_version);

        // Solution ID
        solution_id = HIGH_BYTE(fw_version); 	// Only Needed in Normal Mode
        DEBUG_PRINTF("Solution ID: 0x%02x.\r\n", solution_id);

        //
        // Get & Update Information Page
        //
        err = get_and_update_info_page(solution_id, info_page_buf, sizeof(info_page_buf));
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to get/update Inforamtion Page! err=0x%x.\r\n", __func__, err);
            goto UPDATE_FIRMWARE_EXIT;
        }
    }

    //
    // Remark ID Check
    //
    if(recovery == false) // Normal Mode
    {
        // BC Version (Normal Mode)
        err = get_boot_code_version(&fw_bc_version);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get BC Version (Normal Mode)! err=0x%x.\r\n", __func__, err);
            goto UPDATE_FIRMWARE_EXIT;
        }
        DEBUG_PRINTF("[Normal Mode] BC Version: 0x%04x.\r\n", fw_bc_version);

        // IAP Version
        iap_version = (unsigned char)(fw_bc_version & 0x00FF);
        DEBUG_PRINTF("IAP Version: 0x%02x.\r\n", iap_version);

        // Flag of Remark ID Check
        if(iap_version >= 0x60)
            remark_id_check = true;
        DEBUG_PRINTF("Remark ID Check: %s.\r\n", (remark_id_check) ? "true" : "false");
    }
    else // Recovery Mode
    {
        // BC Version (Recovery Mode)
        err = get_hello_packet_bc_version(&hello_packet, &bc_bc_version);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get BC Version (Recovery Mode)! err=0x%x.\r\n", __func__, err);
            goto UPDATE_FIRMWARE_EXIT;
        }
        DEBUG_PRINTF("[Recovery Mode] BC Version: 0x%04x.\r\n", bc_bc_version);

        // Flag of Remark ID Check
        bc_ver_high_byte = (unsigned char)((bc_bc_version & 0xFF00) >> 8);
        bc_ver_low_byte  = (unsigned char)(bc_bc_version & 0x00FF);
        if(bc_ver_high_byte != bc_ver_low_byte) // EX: A7 60
            remark_id_check = true;
        DEBUG_PRINTF("Remark ID Check: %s.\r\n", (remark_id_check) ? "true" : "false");
    }
    if(remark_id_check == true)
    {
        if(skip_remark_id_check == false) // Check Remark ID
        {
            DEBUG_PRINTF("[%s Mode] Check Remark ID...\r\n", (recovery) ? "Recovery" : "Normal");

            err = check_remark_id(recovery);
            if(err != TP_SUCCESS)
            {
                ERROR_PRINTF("%s: Remark ID Check Failed! err=0x%x.\r\n", __func__, err);
                goto UPDATE_FIRMWARE_EXIT;
            }
        }
        else // Skip Reamrk ID Check, but read Remark ID
        {
            DEBUG_PRINTF("[%s Mode] Read Remark ID...\r\n", (recovery) ? "Recovery" : "Normal");

            err = read_remark_id(recovery);
            if(err != TP_SUCCESS)
            {
                ERROR_PRINTF("%s: Read Remark ID Failed! err=0x%x.\r\n", __func__, err);
                goto UPDATE_FIRMWARE_EXIT;
            }
        }
    }

    //
    // Switch to Boot Code
    //
    err = switch_to_boot_code(recovery);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to switch to Boot Code! err=0x%x.\r\n", __func__, err);
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
        err = write_firmware_page(info_page_buf, sizeof(info_page_buf));
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Write Infomation Page! err=0x%x.\r\n", __func__, err);
            goto UPDATE_FIRMWARE_EXIT;
        }
    }

    // Get FW Size, FW Page Count, and FW Page Block Count
    err = get_firmware_size(&firmware_size);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Firmware Size! err=0x%x.\r\n", __func__, err);
        goto UPDATE_FIRMWARE_EXIT;
    }
    page_count = compute_firmware_page_number(firmware_size);
    block_count = (page_count / 30) + ((page_count % 30) != 0);

    /* [Note] 2022/06/09
     * Reset R/W position of file handler is needed.
     * Since reading remark ID from eKT FW file makes R/W position jump to the last 4-th byte from the end of file.
     * Thus it's need to reset the R/W position to the beginning of the file to read the first page.
     */

    // Reset Read/Write Position of File Handler
    lseek(g_firmware_fd, 0, SEEK_SET);

    // Write Main Pages
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
            ERROR_PRINTF("%s: Fail to Retrieve Page Block Data from Firmware! err=0x%x.\r\n", __func__, err);
            goto UPDATE_FIRMWARE_EXIT;
        }

        // Write Bulk FW Page Data
        err = write_firmware_page(page_block_buf, ELAN_FIRMWARE_PAGE_SIZE * block_page_num);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Write FW Page Block %d (%d-Page)! err=0x%x.\r\n", __func__, block_index, block_page_num, err);
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

