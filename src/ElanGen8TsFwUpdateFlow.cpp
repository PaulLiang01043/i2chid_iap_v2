/** @file

  Header of APIs of Firmware Update Flow for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFwUpdateFlow.cpp

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#include "InterfaceGet.h"
#include "ElanTsFuncApi.h"
#include "ElanTsFwFileIoUtility.h"
#include "ElanTsFwUpdateFlow.h"
#include "ElanGen8TsFuncApi.h"
#include "ElanGen8TsFwFileIoUtility.h"
#include "ElanGen8TsFwUpdateFlow.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

// Firmware Information
int gen8_get_firmware_information(bool silent_mode)
{
    int err = TP_SUCCESS;
    unsigned short fw_id = 0,
                   fw_version = 0,
                   test_version = 0,
                   bc_version = 0;

    if(silent_mode == true) // Enable Silent Mode
    {
        // Firmware Version
        err = get_fw_version(&fw_version);
        if(err != TP_SUCCESS)
            goto GEN8_GET_FW_INFO_EXIT;
        printf("%04x", fw_version);
    }
    else // Normal Mode
    {
        printf("--------------------------------\r\n");

        // FW ID
        err = get_firmware_id(&fw_id);
        if(err != TP_SUCCESS)
            goto GEN8_GET_FW_INFO_EXIT;
        printf("Firmware ID: %02x.%02x\r\n", HIGH_BYTE(fw_id), LOW_BYTE(fw_id));

        // Firmware Version
        err = get_fw_version(&fw_version);
        if(err != TP_SUCCESS)
            goto GEN8_GET_FW_INFO_EXIT;
        printf("Firmware Version: %02x.%02x\r\n", HIGH_BYTE(fw_version), LOW_BYTE(fw_version));

        // Test Version
        err = gen8_get_test_version(&test_version);
        if(err != TP_SUCCESS)
            goto GEN8_GET_FW_INFO_EXIT;
        printf("Test Version: %02x.%02x\r\n", HIGH_BYTE(test_version), LOW_BYTE(test_version));

        // Boot Code Version
        err = get_boot_code_version(&bc_version);
        if(err != TP_SUCCESS)
            goto GEN8_GET_FW_INFO_EXIT;
        printf("Boot Code Version: %02x.%02x\r\n", HIGH_BYTE(bc_version), LOW_BYTE(bc_version));
    }

GEN8_GET_FW_INFO_EXIT:
    return err;
}

// Remark ID Check
int gen8_check_remark_id(bool recovery)
{
    int err = TP_SUCCESS;
    unsigned char gen8_remark_id_from_rom[ELAN_GEN8_REMARK_ID_LEN] = {0},
            gen8_remark_id_from_ektl_fw[ELAN_GEN8_REMARK_ID_LEN] = {0};

    // Get Remark ID from ROM
    err = gen8_read_remark_id(gen8_remark_id_from_rom, sizeof(gen8_remark_id_from_rom), recovery);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Gen8 Remark ID from ROM! err=0x%x.\r\n", __func__, err);
        goto GEN8_CHECK_REMARK_ID_EXIT;
    }

    // Read Remark ID from Firmware
    err = get_remark_id_from_ektl_firmware(gen8_remark_id_from_ektl_fw, sizeof(gen8_remark_id_from_ektl_fw));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Gen8 Remark ID from Firmware! err=0x%x.\r\n", __func__, err);
        goto GEN8_CHECK_REMARK_ID_EXIT;
    }

#ifdef __ENABLE_GEN8_REMARK_ID_CHECK__
    // Check if These Two Remark ID are the Same
    if(memcmp(gen8_remark_id_from_rom, gen8_remark_id_from_ektl_fw, ELAN_GEN8_REMARK_ID_LEN) != 0)
    {
        err = TP_ERR_DATA_MISMATCHED;
        ERROR_PRINTF("%s: Gen8 Remark ID Mismatched! err=0x%x.\r\n", __func__, err);
        goto GEN8_CHECK_REMARK_ID_EXIT;
    }
#endif //__ENABLE_GEN8_REMARK_ID_CHECK__

    // Success
    err = TP_SUCCESS;

GEN8_CHECK_REMARK_ID_EXIT:
    return err;
}

// Firmware Update
int gen8_update_firmware(char *filename, size_t filename_len, bool recovery, int skip_action_code)
{
    int err = TP_SUCCESS,
        firmware_size = 0,
        ektl_fw_page_count = 0,
        ektl_fw_page_index = 0;
    unsigned char ektl_fw_info_page_buf[ELAN_EKTL_FW_PAGE_SIZE] = {0},
                  ektl_fw_page_buf[ELAN_EKTL_FW_PAGE_SIZE] = {0};
    bool skip_remark_id_check = false,
         skip_information_update = false;
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
        goto GEN8_UPDATE_FIRMWARE_EXIT;
    }

    // Make Sure Filename Length Valid
    if(filename_len == 0)
    {
        ERROR_PRINTF("%s: Filename String Length is Zero!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_UPDATE_FIRMWARE_EXIT;
    }

    // Make Sure File Exist
    if(access(filename, F_OK) == -1)
    {
        ERROR_PRINTF("%s: File \"%s\" does not exist!\r\n", __func__, filename);
        err = TP_ERR_FILE_NOT_FOUND;
        goto GEN8_UPDATE_FIRMWARE_EXIT;
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
        // Get & Update Information Page
        DEBUG_PRINTF("Get & Update eKTL FW Information Page...\r\n");
        err = gen8_get_and_update_info_page(ektl_fw_info_page_buf, sizeof(ektl_fw_info_page_buf));
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get & Update eKTL FW Inforamtion Page! err=0x%x.\r\n", __func__, err);
            goto GEN8_UPDATE_FIRMWARE_EXIT;
        }
    }

    //
    // Remark ID Check
    //
    if(skip_remark_id_check == false) // Not Skip Remark ID Check
    {
        DEBUG_PRINTF("Check Gen8 Remark ID...\r\n");

        err = gen8_check_remark_id(recovery);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Gen8 Remark ID Check Failed! err=0x%x.\r\n", __func__, err);
            goto GEN8_UPDATE_FIRMWARE_EXIT;
        }
    }

    //
    // Switch to Boot Code
    //
    err = gen8_switch_to_boot_code(recovery);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Switch to Boot Code! err=0x%x.\r\n", __func__, err);
        goto GEN8_UPDATE_FIRMWARE_EXIT;
    }

    //
    // Erase Flash
    //

    // Flash Sections from eKTL Header
    DEBUG_PRINTF("Erase Flash...\r\n");
    err = erase_flash();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Erase Flash! err=0x%x.\r\n", __func__, err);
        goto GEN8_UPDATE_FIRMWARE_EXIT;
    }

    // Information Page
    if((recovery == false) && (skip_information_update == false))
    {
        DEBUG_PRINTF("Erase Information Flash...\r\n");
        err = erase_info_page_flash();
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Erase Information Page Flash! err=0x%x.\r\n", __func__, err);
            goto GEN8_UPDATE_FIRMWARE_EXIT;
        }
    }

    printf("Start Gen8 FW Update Process...\r\n");

    //
    // Update with eKTL FW Pages
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
        DEBUG_PRINTF("Write eKTL FW Information Page...\r\n");
        err = write_ektl_fw_page(ektl_fw_info_page_buf, sizeof(ektl_fw_info_page_buf));
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Write eKTL FW Information Page! err=0x%x.\r\n", __func__, err);
            goto GEN8_UPDATE_FIRMWARE_EXIT;
        }
    }

    // Get FW Size
    err = get_firmware_size(&firmware_size);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Firmware Size! err=0x%x.\r\n", __func__, err);
        goto GEN8_UPDATE_FIRMWARE_EXIT;
    }

    // Get eKTL FW Page Count (NOT including Header Page)
    ektl_fw_page_count = compute_ektl_fw_page_number(firmware_size);

    // Write $(ektl_fw_page_count) eKTL FW Pages to Touch Flash
    DEBUG_PRINTF("%s: Update with %d eKTL FW Pages...\r\n", __func__, ektl_fw_page_count);
    for(ektl_fw_page_index = 0; ektl_fw_page_index < ektl_fw_page_count; ektl_fw_page_index++)
    {
        // Print test progress to inform operators
        printf(".");
        fflush(stdout);

        // Clear eKTL FW Page Buffer
        memset(ektl_fw_page_buf, 0, sizeof(ektl_fw_page_buf));

        // Load eKTL FW Page Data to Buffer
        err = retrieve_data_from_firmware(ektl_fw_page_buf, ELAN_EKTL_FW_PAGE_SIZE);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Retrieve eKTL FW Page Data from eKTL Firmware! err=0x%x.\r\n", __func__, err);
            goto GEN8_UPDATE_FIRMWARE_EXIT;
        }

        // Write eKTL FW Page Data to Touch
        err = write_ektl_fw_page(ektl_fw_page_buf, sizeof(ektl_fw_page_buf));
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Write %d-th eKTL FW Page Data! err=0x%x.\r\n", __func__, ektl_fw_page_index, err);
            goto GEN8_UPDATE_FIRMWARE_EXIT;
        }
    }

    //
    // Self-Reset
    //

    /* [Note] 2022/06/06
     * With the information from Boot Code Team, it takes 520ms for touch to process after all firmware page data received.
     * Thus it should work to reserve a waiting time of 700ms for safety reasons.
     */
    usleep(700 * 1000); // wait 700ms

    printf("\r\n"); //Print CRLF in console

    // Success
    printf("Gen8 FW Update Finished.\r\n");
    err = TP_SUCCESS;

GEN8_UPDATE_FIRMWARE_EXIT:

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

