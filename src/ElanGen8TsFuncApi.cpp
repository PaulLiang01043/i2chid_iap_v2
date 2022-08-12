/** @file

  Implementation of Function APIs for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFuncApi.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include <time.h>       		/* time_t, struct tm, time, localtime, asctime */
#include "InterfaceGet.h"
#include "ElanTsI2chidUtility.h"
#include "ElanGen8TsI2chidUtility.h"
#include "ElanGen8TsFwFileIoUtility.h"
#include "ElanGen8TsFuncApi.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

// Firmware Information
int gen8_get_test_version(unsigned short *p_test_version)
{
    int err = TP_SUCCESS;
    unsigned short test_version = 0;

    // Send Test Version Command
    err = send_test_version_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Test Version Command! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_TEST_VERSION_EXIT;
    }

    // Receive Test Version Data
    err = gen8_get_test_version_data(&test_version);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive Test Version Data! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_TEST_VERSION_EXIT;
    }

    *p_test_version = test_version;
    err = TP_SUCCESS;

GEN8_GET_TEST_VERSION_EXIT:
    return err;
}

// IAP
int gen8_switch_to_boot_code(bool recovery)
{
    int err = TP_SUCCESS;

    // Enter IAP Mode
    if(recovery == false) // Normal IAP
    {
        // Enter IAP Mode
        err = send_enter_iap_command();
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: [Normal IAP] Fail to Enter IAP Mode! err=0x%x.\r\n", __func__, err);
            goto GEN8_SWITCH_TO_BOOT_CODE_EXIT;
        }

        // wait 30ms
        usleep(30*1000);

        // Gen8 Write Flash Key
        err = send_gen8_write_flash_key_command();
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: [Normal IAP] Fail to Write Flash Key (Gen8)! err=0x%x.\r\n", __func__, err);
            goto GEN8_SWITCH_TO_BOOT_CODE_EXIT;
        }
    }
    else // Recovery IAP
    {
        /* [Note] 2020/02/10
         * Do Not Send Enter IAP Command if Recovery Mode!
         * Just Send Write Flash Key Command!
         * Migrate from V81.
         */

        // Gen8 Write Flash Key
        err = send_gen8_write_flash_key_command();
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: [Recovery IAP] Fail to Write Flash Key (Gen8)! err=0x%x.\r\n", __func__, err);
            goto GEN8_SWITCH_TO_BOOT_CODE_EXIT;
        }
    }

    // wait 15ms
    usleep(15*1000);

    // Check Slave Address
    err = check_slave_address();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Check Slave Address! err=0x%x.\r\n", __func__, err);
        goto GEN8_SWITCH_TO_BOOT_CODE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

GEN8_SWITCH_TO_BOOT_CODE_EXIT:
    return err;
}

// Erase Flash
int erase_flash_section(unsigned int address, unsigned short page_count)
{
    int err = TP_SUCCESS;

    // Valid Page Count to Erase
    if(page_count == 0)
    {
        ERROR_PRINTF("%s: Invalid Page Count (0)!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto ERASE_FLASH_SECTION_EXIT;
    }

    // Request Erase Flash Section
    err = send_erase_flash_section_command(address, page_count);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Request Erase Flash Section! err=0x%x.\r\n", __func__, err);
        goto ERASE_FLASH_SECTION_EXIT;
    }

    /* [Note] 2022/06/02
     * Information From Boot Code Team:
     * Command 0x20: It costs 101ms for  1~32  Pages.
     *						  202ms		33~64  Pages.
     *						  303ms		65~96  Pages.
     *						  404ms		97~132 Pages.
     * Therefore just wait 500ms to be on the safe side.
     */
    usleep(500 * 1000); // wait 500ms

    // Receive Response of Erase Flash Section
    err = receive_erase_flash_section_response();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive Response of Erase Flash Section! err=%d.\r\n", __func__, err);
        goto ERASE_FLASH_SECTION_EXIT;
    }

    // Success
    err = TP_SUCCESS;

ERASE_FLASH_SECTION_EXIT:
    return err;
}

// Erase Flash
int erase_flash(void)
{
    int err = TP_SUCCESS;
    unsigned int erase_section_index = 0,
                 erase_section_address = 0;
    unsigned short erase_section_page_count = 0;
    struct erase_script EraseScript;

    // Initialize Erase Script
    memset(&EraseScript, 0, sizeof(struct erase_script));

    // Get Erase Script
    err = get_ektl_erase_script(&EraseScript, sizeof(struct erase_script));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Erase Script! err=0x%x.\r\n", __func__, err);
        goto ERASE_FLASH_EXIT;
    }

    // Erase Flash Sections
    for(erase_section_index = 0; erase_section_index < EraseScript.nEraseSectionCount; erase_section_index++)
    {
        erase_section_address		= EraseScript.EraseSection[erase_section_index].address;
        erase_section_page_count	= EraseScript.EraseSection[erase_section_index].page_count;
        DEBUG_PRINTF("%s: Erase Flash Section [%d] (address=0x%08x, page_count=%d).\r\n", __func__, \
                     erase_section_index, erase_section_address, erase_section_page_count);

        // Erase Flash Section
        err = erase_flash_section(erase_section_address, erase_section_page_count);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Erase Flash Section [%d]! err=0x%x.\r\n", __func__, err, erase_section_index);
            goto ERASE_FLASH_EXIT;
        }
    }

    // Success
    err = TP_SUCCESS;

ERASE_FLASH_EXIT:
    return err;
}

// Erase Information Page Flash
int erase_info_page_flash(void)
{
    int err = TP_SUCCESS;
    unsigned int erase_section_address = ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR;
    unsigned short erase_section_page_count = 1;

    // Erase Information Page
    err = erase_flash_section(erase_section_address, erase_section_page_count);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Erase Flash Section of Information Page (Address: 0x%08x, Page: %d)! err=0x%x.\r\n", __func__, \
                     erase_section_address, erase_section_page_count, err);
    }

    return err;
}

// eKTL FW Page
int create_ektl_fw_page(unsigned int mem_page_address, unsigned char *p_ektl_fw_page_data_buf, size_t ektl_fw_page_data_buf_size, unsigned char *p_ektl_fw_page_buf, size_t ektl_fw_page_buf_size)
{
    int err = TP_SUCCESS;
    unsigned int  data_index = 0,
                  page_data = 0,
                  page_data_index = 0,
                  page_data_checksum = 0;
    unsigned char ektl_fw_page_buf[ELAN_EKTL_FW_PAGE_SIZE] = {0};

    //
    // Validate Arguments
    //

    // FW Page Buffer Page Data
    if((p_ektl_fw_page_data_buf == NULL) || (ektl_fw_page_data_buf_size < ELAN_EKTL_FW_PAGE_DATA_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_ektl_fw_page_data_buf=0x%p, ektl_fw_page_data_buf_size=%ld)\r\n", __func__, p_ektl_fw_page_data_buf, ektl_fw_page_data_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto CREATE_EKTL_FW_PAGE_EXIT;
    }

    // FW Page Buffer Page
    if((p_ektl_fw_page_buf == NULL) || (ektl_fw_page_buf_size < ELAN_EKTL_FW_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_ektl_fw_page_buf=0x%p, ektl_fw_page_buf_size=%ld)\r\n", __func__, p_ektl_fw_page_buf, ektl_fw_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto CREATE_EKTL_FW_PAGE_EXIT;
    }

    //
    // Setup Firmware Page Buffer
    //

    // Page Address
    ektl_fw_page_buf[0]	= (unsigned char) (mem_page_address & 0x000000FF);
    ektl_fw_page_buf[1]	= (unsigned char)((mem_page_address & 0x0000FF00) >> 8);
    ektl_fw_page_buf[2]	= (unsigned char)((mem_page_address & 0x00FF0000) >> 16);
    ektl_fw_page_buf[3]	= (unsigned char)((mem_page_address & 0xFF000000) >> 24);
    DEBUG_PRINTF("%s: fw_page_buf[0] = {%02x %02x %02x %02x ...}.\r\n", __func__, \
                 ektl_fw_page_buf[0],   ektl_fw_page_buf[1], ektl_fw_page_buf[2], ektl_fw_page_buf[3]);

    // Firmware Page Data
    memcpy(&ektl_fw_page_buf[4], p_ektl_fw_page_data_buf, ektl_fw_page_data_buf_size);
    DEBUG_PRINTF("%s: fw_page_buf[4] = {%02x %02x %02x %02x %02x %02x %02x %02x ...}.\r\n", __func__, \
                 ektl_fw_page_buf[4], ektl_fw_page_buf[5], ektl_fw_page_buf[6],  ektl_fw_page_buf[7], \
                 ektl_fw_page_buf[8], ektl_fw_page_buf[9], ektl_fw_page_buf[10], ektl_fw_page_buf[11]);

    // Calculate Checksum
    for(page_data_index = 0; page_data_index < (4 /* address */ + ELAN_EKTL_FW_PAGE_DATA_SIZE); page_data_index += 4)
    {
        // Get Page Data
        page_data = FOUR_BYTE_ARRAY_TO_UINT(&ektl_fw_page_buf[page_data_index]);

        // Update Checksum
        page_data_checksum += page_data;
    }
    //DEBUG_PRINTF("%s: Checksum=0x%08x.\r\n", __func__, page_data_checksum);

    // Checksum
    data_index = 4 + ELAN_EKTL_FW_PAGE_DATA_SIZE;
    ektl_fw_page_buf[data_index]		= (unsigned char) (page_data_checksum & 0x000000FF);
    ektl_fw_page_buf[data_index + 1]	= (unsigned char)((page_data_checksum & 0x0000FF00) >> 8);
    ektl_fw_page_buf[data_index + 2]	= (unsigned char)((page_data_checksum & 0x00FF0000) >> 16);
    ektl_fw_page_buf[data_index + 3]	= (unsigned char)((page_data_checksum & 0xFF000000) >> 24);
    DEBUG_PRINTF("%s: fw_page_buf[%d] = {%02x %02x %02x %02x}.\r\n", __func__, data_index, \
                 ektl_fw_page_buf[data_index],     ektl_fw_page_buf[data_index + 1], \
                 ektl_fw_page_buf[data_index + 2], ektl_fw_page_buf[data_index + 3]);

    // Load Data of Firmware Page to Input Buffer
    memcpy(p_ektl_fw_page_buf, ektl_fw_page_buf, sizeof(ektl_fw_page_buf));

    // Success
    err = TP_SUCCESS;

CREATE_EKTL_FW_PAGE_EXIT:
    return err;
}

int write_ektl_fw_page(unsigned char *p_ektl_fw_page_buf, size_t ektl_fw_page_buf_size)
{
    int err = TP_SUCCESS,
        frame_index = 0,
        frame_count = 0,
        frame_data_len = 0,
        start_index = 0;
    unsigned char temp_ektl_fw_page_buf[ELAN_EKTL_FW_PAGE_SIZE] = {0};

    // Valid Input eKTL FW Page Buffer
    if(p_ektl_fw_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL eKTL FW Page Buffer Pointer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto WRITE_EKTL_FW_PAGE_EXIT;
    }

    // Validate Input eKTL FW Page Buffer Size
    if((ektl_fw_page_buf_size == 0) || (ektl_fw_page_buf_size > ELAN_EKTL_FW_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Page Buffer Size: %ld.\r\n", __func__, ektl_fw_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto WRITE_EKTL_FW_PAGE_EXIT;
    }

    // Copy eKTL FW Page Data from Input FW Page Buffer to Temp FW Page Buffer
    memcpy(temp_ektl_fw_page_buf, p_ektl_fw_page_buf, ektl_fw_page_buf_size);

    // Get Frame Count
    frame_count = (ektl_fw_page_buf_size / ELAN_I2CHID_PAGE_FRAME_SIZE) +
                  ((ektl_fw_page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE) != 0);

    // Write eKTL FW Page Data with Frames
    for(frame_index = 0; frame_index < frame_count; frame_index++)
    {
        if((frame_index == (frame_count - 1)) && ((ektl_fw_page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE) > 0)) // The Last Frame
            frame_data_len = ektl_fw_page_buf_size % ELAN_I2CHID_PAGE_FRAME_SIZE;
        else
            frame_data_len = ELAN_I2CHID_PAGE_FRAME_SIZE;

        // Write Frame Data
        err = write_frame_data(start_index, frame_data_len, &temp_ektl_fw_page_buf[start_index], frame_data_len);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Update eKTL FW Page from 0x%x-th Frame (Frame Data Length %d)! err=%d.\r\n", __func__, start_index, frame_data_len, err);
            goto WRITE_EKTL_FW_PAGE_EXIT;
        }

        // Update Start Index to Next Frame
        start_index += frame_data_len;
    }

    // Request Flash Write
    err = send_flash_write_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Request Flash Write! err=0x%x.\r\n", __func__, err);
        goto WRITE_EKTL_FW_PAGE_EXIT;
    }

    // Wait for FW Writing Flash

    /* [Note] 2022/06/06
     * With the information from Boot Code Team, it takes 7ms for touch to process after receiving firmware page data.
     * Thus it should work to remain waiting time of 15ms.
     */
    usleep(15 * 1000); // wait 15ms

    // Receive Response of Flash Write
    err = receive_flash_write_response();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive Flash Write! err=0x%x.\r\n", __func__, err);
        goto WRITE_EKTL_FW_PAGE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

WRITE_EKTL_FW_PAGE_EXIT:
    return err;
}

// ROM Data
int gen8_get_rom_data(unsigned int addr, unsigned char data_len, unsigned int *p_data)
{
    int err = TP_SUCCESS;
    unsigned int data = 0;

    // Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_ROM_DATA_EXIT;
    }

    /* Read Data from ROM */

    // Send New Read ROM Data Command (New 0x96 Command)
    err = gen8_send_read_rom_data_command(addr, data_len);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send New Read ROM Data Command! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_ROM_DATA_EXIT;
    }

    // Receive ROM Data
    err = gen8_receive_rom_data(&data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive ROM Data! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_ROM_DATA_EXIT;
    }

    // Load ROM Data to Input Buffer
    *p_data = data;

    // Success
    err = TP_SUCCESS;

GEN8_GET_ROM_DATA_EXIT:
    return err;
}

// Remark ID
int gen8_read_remark_id(unsigned char *p_gen8_remark_id_buf, size_t gen8_remark_id_buf_size, bool recovery)
{
    int err = TP_SUCCESS;
    unsigned char gen8_remark_id_data[ELAN_GEN8_REMARK_ID_LEN] = {0},
            gen8_remark_id_index = 0,
            data_index = 0,
            data_count = 0;
    unsigned int rom_data = 0,
                 gen8_remark_id_address = 0,
                 gen8_remark_id_data_address = 0,
                 gen8_remark_id_address_set[7] = {0x00042240 /* Address Set 1: 0x00042240 ~ 0x0004227C */,
                                                  0x00042280 /* Address Set 2: 0x00042280 ~ 0x000422BC */,
                                                  0x000422C0 /* Address Set 3: 0x000422C0 ~ 0x000422FC */,
                                                  0x00042300 /* Address Set 4: 0x00042300 ~ 0x0004233C */,
                                                  0x00042340 /* Address Set 5: 0x00042340 ~ 0x0004237C */,
                                                  0x00042380 /* Address Set 6: 0x00042380 ~ 0x000423BC */,
                                                  0x000423C0 /* Address Set 7: 0x000423C0 ~ 0x000423FC */
                                                 };

    // Check if Parameter Invalid
    if ((p_gen8_remark_id_buf == NULL) || (gen8_remark_id_buf_size < ELAN_GEN8_REMARK_ID_LEN))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_gen8_remark_id_buf=0x%p, gen8_remark_id_buf_size=%ld)\r\n", __func__, p_gen8_remark_id_buf, gen8_remark_id_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_READ_REMARK_ID_EXIT;
    }

    /*
     * Remark ID Index
     */

    // Read ROM Data from Remark ID Index Address (0x00042200)
    err = gen8_get_rom_data(ELAN_GEN8_REMARK_ID_INDEX_ADDR, 1, &rom_data); // Byte Data / 8-bit
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get ROM Data of MEM[0x%08x]! err=0x%x.\r\n", \
                     __func__, ELAN_GEN8_REMARK_ID_INDEX_ADDR, err);
        goto GEN8_READ_REMARK_ID_EXIT;
    }

    // 2's Complement of ROM Data
    gen8_remark_id_index = 0xFF - LOW_BYTE(rom_data);

    /*
     * Remark ID Data
     */

    // Remark ID Address
    gen8_remark_id_address = gen8_remark_id_address_set[gen8_remark_id_index];
    DEBUG_PRINTF("%s: Gen8 Remark ID Address [%d] = 0x%08x.\r\n", __func__, gen8_remark_id_address, gen8_remark_id_address);

    // Read ROM Data from Remark ID Address $(gen8_remark_id_address)
    data_count = ELAN_GEN8_REMARK_ID_LEN; // 16-byte
    for(data_index = 0; data_index < data_count; data_index++)
    {
        gen8_remark_id_data_address = gen8_remark_id_address + (4 * data_index);
        rom_data = 0;

        err = gen8_get_rom_data(gen8_remark_id_data_address, 1, &rom_data); // Byte Data / 8-bit
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get ROM Data of MEM[0x%08x]! err=0x%x.\r\n", \
                         __func__, gen8_remark_id_data_address, err);
            goto GEN8_READ_REMARK_ID_EXIT;
        }
        gen8_remark_id_data[data_index] = LOW_BYTE(rom_data);
        DEBUG_PRINTF("%s: MEM[0x%08x] = 0x%02x.\r\n", __func__, gen8_remark_id_data_address, gen8_remark_id_data[data_index]);
    }

    DEBUG_PRINTF("%s: Gen8 Remark ID [%d] = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x.\r\n", \
                 __func__, gen8_remark_id_index, \
                 gen8_remark_id_data[0],  gen8_remark_id_data[1],  gen8_remark_id_data[2],  gen8_remark_id_data[3],  \
                 gen8_remark_id_data[4],  gen8_remark_id_data[5],  gen8_remark_id_data[6],  gen8_remark_id_data[7],  \
                 gen8_remark_id_data[8],  gen8_remark_id_data[9],  gen8_remark_id_data[10], gen8_remark_id_data[11], \
                 gen8_remark_id_data[12], gen8_remark_id_data[13], gen8_remark_id_data[14], gen8_remark_id_data[15]);

    // Load ROM Data to Input Buffer
    memcpy(p_gen8_remark_id_buf, gen8_remark_id_data, sizeof(gen8_remark_id_data));

    // Success
    err = TP_SUCCESS;

GEN8_READ_REMARK_ID_EXIT:
    return err;
}

// Page Data
int gen8_read_memory_page(unsigned short mem_page_address, unsigned short mem_page_size, unsigned char *p_mem_page_buf, size_t mem_page_buf_size)
{
    int err = TP_SUCCESS;
    unsigned int page_frame_index = 0,
                 page_frame_count = 0,
                 page_frame_data_len = 0,
                 data_len = 0,
                 page_data_index = 0;
    unsigned char gen8_mem_page_buf[ELAN_GEN8_MEMORY_PAGE_SIZE] = {0},
            data_buf[ELAN_I2CHID_DATA_BUFFER_SIZE] = {0};

    //
    // Validate Arguments
    //

    // Make Sure Page Data Buffer Valid
    if(p_mem_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_READ_MEMORY_PAGE_EXIT;
    }

    // Make Sure Page Data Buffer Size Valid
    if(mem_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE)
    {
        ERROR_PRINTF("%s: Invalid Memory Page Buffer Size (%ld)!\r\n", __func__, mem_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_READ_MEMORY_PAGE_EXIT;
    }

    // Send Show Bulk ROM Data Command
    err = send_show_bulk_rom_data_command(mem_page_address, mem_page_size /* (Gen8) unit: byte */);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Gen8 Show Bulk ROM Data Command! err=0x%x.\r\n", __func__, err);
        goto GEN8_READ_MEMORY_PAGE_EXIT;
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
        else // (page_frame_index != (page_frame_count -1)) || ((ELAN_FIRMWARE_PAGE_DATA_SIZE % ELAN_I2CHID_READ_PAGE_FRAME_SIZE) == 0)
            page_frame_data_len = ELAN_I2CHID_READ_PAGE_FRAME_SIZE;
        data_len = 3 /* 1(Packet Header 0x99) + 1(Packet Index) + 1(Data Length) */ + page_frame_data_len;

        // Read $(page_frame_index)-th Bulk Page Data to Buffer
        err = read_data(data_buf, data_len, ELAN_READ_DATA_TIMEOUT_MSEC);
        if(err != TP_SUCCESS) // Error or Timeout
        {
            ERROR_PRINTF("%s: [%d] Fail to Read %d-Byte Data! err=0x%x.\r\n", __func__, page_frame_index, data_len, err);
            goto GEN8_READ_MEMORY_PAGE_EXIT;
        }

        // Copy Read Data to Page Buffer
        memcpy(&gen8_mem_page_buf[page_data_index], &data_buf[3], page_frame_data_len);
        page_data_index += page_frame_data_len;
    }

    // Copy Page Data to Input Buffer
    memcpy(p_mem_page_buf, gen8_mem_page_buf, sizeof(gen8_mem_page_buf));

    // Success
    err = TP_SUCCESS;

GEN8_READ_MEMORY_PAGE_EXIT:
    return err;
}

// Information Page
int gen8_get_info_page(unsigned char *p_info_page_buf, size_t info_page_buf_size)
{
    int err = TP_SUCCESS;
    unsigned short gen8_mem_addr_offset = 0;
    unsigned char gen8_info_mem_page_buf[ELAN_GEN8_MEMORY_PAGE_SIZE] = {0};

    //
    // Validate Arguments
    //

    // Info. Page Buffer
    if(p_info_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL Info. Page Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_EXIT;
    }

    // Info. Page Buffer Size
    if(info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE)
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer Size (%ld)!\r\n", __func__, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_EXIT;
    }

    //
    // Read Information Memory Page
    //

    // Enter Test Mode
    err = send_enter_test_mode_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Enter Test Mode! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_INFO_PAGE_EXIT;
    }

    // Read Information Page
    gen8_mem_addr_offset = ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR - ELAN_GEN8_INFO_ROM_MEMORY_ADDR;
    err = gen8_read_memory_page(gen8_mem_addr_offset, ELAN_GEN8_MEMORY_PAGE_SIZE, gen8_info_mem_page_buf, sizeof(gen8_info_mem_page_buf));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Information Page! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_INFO_PAGE_EXIT_1;
    }

    // Leave Test Mode
    err = send_exit_test_mode_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Leave Test Mode! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_INFO_PAGE_EXIT;
    }

    // Load Information Page Data to Input Buffer
    memcpy(p_info_page_buf, gen8_info_mem_page_buf, sizeof(gen8_info_mem_page_buf));

    // Success
    err = TP_SUCCESS;

GEN8_GET_INFO_PAGE_EXIT:
    return err;

GEN8_GET_INFO_PAGE_EXIT_1:
    // Leave Test Mode
    send_exit_test_mode_command();

    return err;
}

int gen8_get_info_page_with_error_retry(unsigned char *p_info_page_buf, size_t info_page_buf_size, int retry_count)
{
    int err = TP_SUCCESS,
        retry_index = 0;

    //
    // Validate Arguments
    //

    // Info. Page Buffer
    if(p_info_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL Info. Page Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_WITH_ERROR_RETRY_EXIT;
    }

    // Info. Page Buffer Size
    if(info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE)
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer Size! (info_page_buf_size=%ld)\r\n", __func__, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_WITH_ERROR_RETRY_EXIT;
    }

    // Make Sure Retry Count Positive
    if(retry_count <= 0)
        retry_count = 1;

    //
    // Read Information Page
    //
    for(retry_index = 0; retry_index < retry_count; retry_index++)
    {
        err = gen8_get_info_page(p_info_page_buf, info_page_buf_size);
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
            goto GEN8_GET_INFO_PAGE_WITH_ERROR_RETRY_EXIT;
        }
        else // retry_index = 0, 1
        {
            // wait 50ms
            usleep(50*1000);

            continue;
        }
    }

GEN8_GET_INFO_PAGE_WITH_ERROR_RETRY_EXIT:
    return err;
}

// Update Info.
int gen8_get_update_info(unsigned char *p_info_page_buf, size_t info_page_buf_size, struct update_info *p_update_info, size_t update_info_size)
{
    int err = TP_SUCCESS;
    unsigned int gen8_update_counter			= 0,
                 gen8_last_update_time_year		= 0,
                 gen8_last_update_time_month	= 0,
                 gen8_last_update_time_day		= 0,
                 gen8_last_update_time_hour		= 0,
                 gen8_last_update_time_minute	= 0;
    struct update_info last_update_info;

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    // Update Info. & Update Info. Size
    if((p_update_info == NULL) || (update_info_size < sizeof(struct update_info)))
    {
        ERROR_PRINTF("%s: Invalid Update Info.! (p_update_info=0x%p, update_info_size=%ld)\r\n", \
                     __func__, p_update_info, update_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    //
    // Get Update Inforamtion
    //

    // Update Counter
    err = gen8_get_info_page_value(p_info_page_buf, info_page_buf_size, ELAN_GEN8_UPDATE_COUNTER_ADDR, &gen8_update_counter);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Update Counter (Address: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_UPDATE_COUNTER_ADDR, err);
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }
    if(gen8_update_counter == 0xFFFFFFFF)
        gen8_update_counter = 0;


    // Last Update Time (Year)
    err = gen8_get_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR, &gen8_last_update_time_year);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Year (Address: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR, err);
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    // Last Update Time (Month)
    err = gen8_get_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR, &gen8_last_update_time_month);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Month (Address: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR, err);
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    // Last Update Time (Day)
    err = gen8_get_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR, &gen8_last_update_time_day);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Hour (Address: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR, err);
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    // Last Update Time (Hour)
    err = gen8_get_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR, &gen8_last_update_time_hour);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Hour (Address: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR, err);
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    // Last Update Time (Minute)
    err = gen8_get_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR, &gen8_last_update_time_minute);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Last Update Time - Minute (Address: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR, err);
        goto GEN8_GET_UPDATE_INFO_EXIT;
    }

    // Setup Last Update Info. Container
    last_update_info.update_counter				= gen8_update_counter;
    last_update_info.last_update_time.Year		= gen8_last_update_time_year;
    last_update_info.last_update_time.Month		= gen8_last_update_time_month;
    last_update_info.last_update_time.Day		= gen8_last_update_time_day;
    last_update_info.last_update_time.Hour		= gen8_last_update_time_hour;
    last_update_info.last_update_time.Minute	= gen8_last_update_time_minute;

    // Load Last Update Info. to Input Buffer
    memcpy(p_update_info, &last_update_info, sizeof(last_update_info));

    // Success
    err = TP_SUCCESS;

GEN8_GET_UPDATE_INFO_EXIT:
    return err;
}

int gen8_set_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int value)
{
    int err 				= TP_SUCCESS;
    unsigned int data_index	= 0; // Memory Address Offset

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_SET_INFO_PAGE_VALUE_EXIT;
    }

    // Address
    if(address < ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%08x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_SET_INFO_PAGE_VALUE_EXIT;
    }

    //
    // Set Value
    //

    // Data Index
    data_index = address - ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR;

    // Load Data Value to Appropriate Address
    p_info_page_buf[data_index]		= (unsigned char) (value & 0x000000FF);
    p_info_page_buf[data_index+1]	= (unsigned char)((value & 0x0000FF00) >> 8);
    p_info_page_buf[data_index+2]	= (unsigned char)((value & 0x00FF0000) >> 16);
    p_info_page_buf[data_index+3]	= (unsigned char)((value & 0xFF000000) >> 24);
    DEBUG_PRINTF("%s: address: 0x%08x, value: %d (0x%08x), info_page_buf[0x%08x] = {%02x %02x %02x %02x}.\r\n", \
                 __func__, address, value, value, data_index, \
                 p_info_page_buf[data_index],     p_info_page_buf[data_index + 1], \
                 p_info_page_buf[data_index + 2], p_info_page_buf[data_index + 3]);

    // Success
    err = TP_SUCCESS;

GEN8_SET_INFO_PAGE_VALUE_EXIT:
    return err;
}

int gen8_get_info_page_value(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int *p_value)
{
    int err 				= TP_SUCCESS;
    unsigned int data_index	= 0, // Memory Address Offset
                 data_value	= 0;

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_VALUE_EXIT;
    }

    // Address
    if(address < ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%08x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_VALUE_EXIT;
    }

    // Value Buffer
    if(p_value == NULL)
    {
        ERROR_PRINTF("%s: NULL Value Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_VALUE_EXIT;
    }

    //
    // Get Value
    //

    // Data Index
    data_index = address - ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR;

    // Data Value
    data_value = FOUR_BYTE_ARRAY_TO_UINT(&p_info_page_buf[data_index]);
    DEBUG_PRINTF("%s: MEM[0x%08x] = {0x%x 0x%x 0x%x 0x%x} => 0x%08x (%d).\r\n", \
                 __func__, address, \
                 p_info_page_buf[data_index],     p_info_page_buf[data_index + 1], \
                 p_info_page_buf[data_index + 2], p_info_page_buf[data_index + 3], \
                 data_value, data_value);

    // Load Data Value to Input Value Buffer
    *p_value = data_value;

    // Success
    err = TP_SUCCESS;

GEN8_GET_INFO_PAGE_VALUE_EXIT:
    return err;
}

int gen8_set_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int value)
{
    int err 						= TP_SUCCESS;
    unsigned int data_index			= 0, // Memory Address Offset
                 hex_value_interger	= 0;
    char hex_value_string[16]		= {0}; // Format: "0xabcdefgh"

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_SET_INFO_PAGE_DATA_EXIT;
    }

    // Address
    if(address < ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%08x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_SET_INFO_PAGE_DATA_EXIT;
    }

    //
    // Set Data
    //

    // Data Index
    data_index = address - ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR;

    // Data Value
    sprintf(hex_value_string, "0x%d", value);
    hex_value_interger = (unsigned int)strtoul(hex_value_string, NULL, 0 /* determined by the format of hex_value_string */);

    // Load Data Value to Appropriate Address
    p_info_page_buf[data_index]		= (unsigned char) (hex_value_interger & 0x000000FF);
    p_info_page_buf[data_index+1]	= (unsigned char)((hex_value_interger & 0x0000FF00) >> 8);
    p_info_page_buf[data_index+2]	= (unsigned char)((hex_value_interger & 0x00FF0000) >> 16);
    p_info_page_buf[data_index+3]	= (unsigned char)((hex_value_interger & 0xFF000000) >> 24);
    DEBUG_PRINTF("%s: address: 0x%08x, value: %d (0x%08x), info_page_buf[0x%08x] = {%02x %02x %02x %02x}.\r\n", __func__, \
                 address, value, value, data_index, \
                 p_info_page_buf[data_index],     p_info_page_buf[data_index + 1], \
                 p_info_page_buf[data_index + 2], p_info_page_buf[data_index + 3]);

    // Success
    err = TP_SUCCESS;

GEN8_SET_INFO_PAGE_DATA_EXIT:
    return err;
}

int gen8_get_info_page_data(unsigned char *p_info_page_buf, size_t info_page_buf_size, unsigned int address, unsigned int *p_value)
{
    int err 					= TP_SUCCESS;
    unsigned int int_value		= 0,
                 data_index		= 0, // Memory Address Offset
                 data_value		= 0;
    char int_value_string[16]	= {0}; // Format: "0xabcdefgh"

    //
    // Validate Arguments
    //

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_DATA_EXIT;
    }

    // Address
    if(address < ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR)
    {
        ERROR_PRINTF("%s: Invalid Address of Information Data! (address=0x%08x.)\r\n", __func__, address);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_DATA_EXIT;
    }

    // Value Buffer
    if(p_value == NULL)
    {
        ERROR_PRINTF("%s: NULL Value Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_INFO_PAGE_DATA_EXIT;
    }

    //
    // Get Data
    //

    // Data Index
    data_index = address - ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR;

    // Data Value
    data_value = FOUR_BYTE_ARRAY_TO_UINT(&p_info_page_buf[data_index]);

    // Hex. to Int.
    sprintf(int_value_string, "%x", data_value);
    int_value = (unsigned int)strtoul(int_value_string, NULL, 10);
    DEBUG_PRINTF("%s: MEM[0x%08x] = {0x%x 0x%x 0x%x 0x%x} => 0x%08x => %d.\r\n", \
                 __func__, address, \
                 p_info_page_buf[data_index],     p_info_page_buf[data_index + 1], \
                 p_info_page_buf[data_index + 2], p_info_page_buf[data_index + 3], \
                 data_value, int_value);

    // Load Data Value to Input Value Buffer
    *p_value = int_value;

    // Success
    err = TP_SUCCESS;

GEN8_GET_INFO_PAGE_DATA_EXIT:
    return err;
}

int gen8_update_info_page(struct update_info *p_update_info, size_t update_info_size, unsigned char *p_info_page_buf, size_t info_page_buf_size)
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
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Info. Page Buffer & Buffer Size
    if((p_info_page_buf == NULL) || (info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE))
    {
        ERROR_PRINTF("%s: Invalid Info. Page Buffer! (p_info_page_buf=0x%p, info_page_buf_size=%ld)\r\n", \
                     __func__, p_info_page_buf, info_page_buf_size);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    //
    // Get Update Inforamtion
    //

    // Update Counter
    err = gen8_set_info_page_value(p_info_page_buf, info_page_buf_size, ELAN_GEN8_UPDATE_COUNTER_ADDR, p_update_info->update_counter);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Update Counter (Address: 0x%08x, Value: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_UPDATE_COUNTER_ADDR, p_update_info->update_counter, err);
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Year)
    err = gen8_set_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR, p_update_info->last_update_time.Year);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Year (Address: 0x%08x, Value: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_YEAR_ADDR, p_update_info->last_update_time.Year, err);
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Month)
    err = gen8_set_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR, p_update_info->last_update_time.Month);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Month (Address: 0x%08x, Value: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_MONTH_ADDR, p_update_info->last_update_time.Month, err);
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Day)
    err = gen8_set_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR, p_update_info->last_update_time.Day);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Day (Address: 0x%08x, Value: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_DAY_ADDR, p_update_info->last_update_time.Day, err);
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Hour)
    err = gen8_set_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR, p_update_info->last_update_time.Hour);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Hour (Address: 0x%08x, Value: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_HOUR_ADDR, p_update_info->last_update_time.Hour, err);
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Last Update Time (Minute)
    err = gen8_set_info_page_data(p_info_page_buf, info_page_buf_size, ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR, p_update_info->last_update_time.Minute);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Set Last Update Time - Minute (Address: 0x%08x, Value: 0x%08x)! err=0x%x.\r\n", __func__, \
                     ELAN_GEN8_LAST_UPDATE_TIME_MINUTE_ADDR, p_update_info->last_update_time.Minute, err);
        goto GEN8_UPDATE_INFO_PAGE_EXIT;
    }

    // Success
    err = TP_SUCCESS;

GEN8_UPDATE_INFO_PAGE_EXIT:
    return err;
}

int gen8_get_and_update_info_page(unsigned char *p_info_page_buf, size_t info_page_buf_size)
{
    int err = TP_SUCCESS;
    unsigned char gen8_info_mem_page_buf[ELAN_GEN8_MEMORY_PAGE_SIZE] = {0},
                  ektl_fw_info_page_buf[ELAN_EKTL_FW_PAGE_SIZE] = {0},
                  ektl_fw_info_page_data_buf[ELAN_EKTL_FW_PAGE_DATA_SIZE] = {0};
    time_t cur_time;
    struct tm *time_info;
    struct update_info last_update_info;

    //
    // Validate Arguments
    //

    // Info. Page Buffer
    if(p_info_page_buf == NULL)
    {
        ERROR_PRINTF("%s: NULL Info. Page Buffer!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    // Info. Page Buffer Size
    if(info_page_buf_size < ELAN_GEN8_MEMORY_PAGE_SIZE)
    {
        ERROR_PRINTF("%s: Info. Page Buffer Size is Zero!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    //
    // Get Update Inforamtion
    //

    // Get Information Memory Page Data
    err = gen8_get_info_page_with_error_retry(gen8_info_mem_page_buf, sizeof(gen8_info_mem_page_buf), ERROR_RETRY_COUNT);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get Information Page! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    // Initialize eKTL FW Information Page Data Buffer
    memcpy(ektl_fw_info_page_data_buf, gen8_info_mem_page_buf, sizeof(gen8_info_mem_page_buf));

    // Parse Update Info. from Information Memory Page Data
    err = gen8_get_update_info(gen8_info_mem_page_buf, sizeof(gen8_info_mem_page_buf), &last_update_info, sizeof(last_update_info));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Parse Update Information! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT;
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
    //DEBUG_PRINTF("%s: Current date & time: %s.\r\n", __func__, asctime(time_info));

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
    err = gen8_update_info_page(&last_update_info, sizeof(last_update_info), ektl_fw_info_page_data_buf, sizeof(ektl_fw_info_page_data_buf));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Update Information Page Data! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    //
    // Setup Information Page
    //
    err = create_ektl_fw_page(ELAN_GEN8_INFO_MEMORY_PAGE_3_ADDR, \
                              ektl_fw_info_page_data_buf, sizeof(ektl_fw_info_page_data_buf), \
                              ektl_fw_info_page_buf, sizeof(ektl_fw_info_page_buf));
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Setup Information Page! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT;
    }

    // Load Information Page Data to Input Buffer
    memcpy(p_info_page_buf, ektl_fw_info_page_buf, sizeof(ektl_fw_info_page_buf));

    // Success
    err = TP_SUCCESS;

GEN8_GET_AND_UPDATE_INFO_PAGE_EXIT:
    return err;
}

