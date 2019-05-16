// I2CHIDLinuxGet.cpp : implementation file
//

//#include "stdafx.h"
#include "I2CHIDLinuxGet.h"
#include <fcntl.h>      /* open */
#include <unistd.h>     /* close */
#include <sys/ioctl.h>  /* ioctl */
#include <dirent.h>         // opendir, readdir, closedir
#include <linux/hidraw.h>	// hidraw
#include <linux/input.h>	// BUS_TYPE
#include <errno.h>			// errno
// Debug Utility
#ifdef _WIN32 // Windows 32-bit Platform
#include "win32_debug_utility.h"
#endif // Debug Utility

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::CI2CHIDGetLinux()
// 1. Set Initial Value to Member Variables
// 2. Allocate Memeroy to Resource Needed
// 3. Initialize mutex (semaphore)
// 4. Initialize libusb

CI2CHIDLinuxGet::CI2CHIDLinuxGet(char *pszLogDirPath, char *pszDebugLogFileName) : CBaseLog(pszLogDirPath, pszDebugLogFileName)
{
    //DBG("Construct CI2CHIDLinuxGet.");

    // Initialize hidraw device handler
    m_nHidrawFd = -1;

    // Initialize file descriptor monitor
    memset(&m_tvRead, 0, sizeof(struct timeval));

    // Assign initial values to chip data
    m_usVID = 0;
    m_usPID = 0;
    m_usVersion = 0;

    // Assign Initial values to buffers
    m_inBuf 	= NULL;
    m_inBufSize	= 0;
    m_outBuf	= NULL;
    m_outBufSize	= 0;

    // Initialize mutex
    sem_init(&m_ioMutex,  0 /*scope is in this file*/, 1 /*active in initial*/);

    // Allocate memory to inBuffer
    m_inBufSize = ELAN_I2CHID_INPUT_BUFFER_SIZE;
    //DBG("Allocate %d bytes to inBuffer.", m_inBufSize);
    m_inBuf = (unsigned char*)malloc(sizeof(unsigned char) * m_inBufSize);
    memset(m_inBuf, 0, sizeof(unsigned char)*m_inBufSize);

    // Allocate memory to outBuffer
    m_outBufSize = ELAN_I2CHID_OUTPUT_BUFFER_SIZE;
    //DBG("Allocate %d bytes to outBuffer.", m_outBufSize);
    m_outBuf = (unsigned char*)malloc(sizeof(unsigned char) * m_outBufSize);
    memset(m_outBuf, 0, sizeof(unsigned char)*m_outBufSize);

    return;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::~CI2CHIDLinuxGet()
// 1. Free class members
// 2. Deinitialize mutex (semaphore)
// 3. Deinitialize libusb

CI2CHIDLinuxGet::~CI2CHIDLinuxGet(void)
{
    // Deinitialize mutex (semaphore)
    sem_destroy(&m_ioMutex);

    // Release input buffer
    if (m_inBuf)
    {
        //DBG("Release input buffer (address=%p).\r\n", m_inBuf);
        free(m_inBuf);
        m_inBuf		= NULL;
        m_inBufSize     = 0;
    }

    // Release output buffer
    if (m_outBuf)
    {
        //DBG("Release output buffer (address=%p).\r\n", m_outBuf);
        free(m_outBuf);
        m_outBuf        = NULL;
        m_outBufSize    = 0;
    }

    // Clear Chip Data
    m_usVID = 0;
    m_usPID = 0;
    m_usVersion = 0;

    return;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::Close()
// 1. Close the current hid device
// 2. Reatach the kernel driver previously used
// 2. Release the resource occupied and clear device attributes.

void CI2CHIDLinuxGet::Close(void)
{
    if (m_nHidrawFd >= 0)
    {
        // Release acquired hidraw device handler
        DBG("%s: Release hidraw device handle (fd=%d).", __func__, m_nHidrawFd);
        close(m_nHidrawFd);
        m_nHidrawFd = -1;
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::GetDeviceHandle(int nParam1, int nParam2)
// 1. Connect to hid-raw device
// 2. Open requested hid device with input VID & PID
// 3. Allocate I/O buffer and fill device attibutes
// 4. Print system hid information if debug is enable

int CI2CHIDLinuxGet::GetDeviceHandle(int nVID, int nPID)
{
    int nRet = TP_SUCCESS,
        nError = 0;
    char szHidrawDevPath[64] = {0};

    // Look for elan hidraw device with specific PID
    nError = FindHidrawDevice(nVID, nPID, szHidrawDevPath);
	if(nError == TP_ERR_NOT_FOUND_DEVICE)
	{
		DBG("%s: hidraw device (VID 0x%x, PID 0x%x) not found! Retry with PID 0x%x.", __func__, nVID, nPID, ELAN_USB_FORCE_CONNECT_PID);
		nError = FindHidrawDevice(nVID, ELAN_USB_FORCE_CONNECT_PID, szHidrawDevPath);
		if (nError != TP_SUCCESS)
        {
			ERR("%s: hidraw device (VID 0x%x, PID 0x%x) not found!", __func__, nVID, ELAN_USB_FORCE_CONNECT_PID);
        	nRet = TP_ERR_NOT_FOUND_DEVICE;
        	goto GET_DEVICE_HANDLE_EXIT;
    	}
	}
    else if (nError != TP_SUCCESS)
    {
		ERR("%s: hidraw device (VID 0x%x, PID 0x%x) not found!", __func__, nVID, nPID);
        nRet = TP_ERR_NOT_FOUND_DEVICE;
        goto GET_DEVICE_HANDLE_EXIT;
    }

    // Acquire hidraw device handler for I/O
    nError = open(szHidrawDevPath, O_RDWR | O_NONBLOCK);
    if (nError < 0)
    {
        ERR("%s: Fail to Open Device %s! errno=%d.", __func__, szHidrawDevPath, nError);
        nRet = TP_ERR_NOT_FOUND_DEVICE;
        goto GET_DEVICE_HANDLE_EXIT;
    }

    // Success
    m_nHidrawFd = nError;
    DBG("%s: Open hidraw device \'%s\' (non-blocking), fd=%d.", __func__, szHidrawDevPath, m_nHidrawFd);

GET_DEVICE_HANDLE_EXIT:
    return nRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::IsConnected()
// Check if device connected
bool CI2CHIDLinuxGet::IsConnected(void)
{
    bool bRet = false;

    if (m_nHidrawFd >= 0)
    {
        //DBG("Device is connected!\r\n");
        bRet = true;
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::WriteRawBytes()
// Write Data to HID device
// cBuf: Buffer to write
// nLen: Data length to write
// nTimeout: Time to wait for device respond

int CI2CHIDLinuxGet::WriteRawBytes(unsigned char* pszBuf, int nLen, int nTimeout, int nDevIdx)
{
    int nRet = 0,
        nResult = 0,
        nPollIndex = 0,
        nPollCount = 0;

    nPollCount = nTimeout;

    if ((unsigned)nLen > m_outBufSize)
    {
        ERR("%s: Data length too large(data=%d, buffer size=%d), ret=%d.\r\n", __func__, nLen, m_outBufSize, nRet);
        nRet = TP_ERR_INVALID_PARAM;
        goto WRITE_RAW_BYTES_EXIT;
    }

    // Mutex locks the critical section
    sem_wait(&m_ioMutex);

    // Copy data to local buffer and write buffer data to usb
    memset(m_outBuf, 0, sizeof(unsigned char)*m_outBufSize);
    memcpy(m_outBuf, pszBuf, ((unsigned)nLen <= m_outBufSize) ? nLen : m_outBufSize);

#ifdef __ENABLE_DEBUG__
    if (g_bEnableDebug)
        DebugPrintBuffer("m_outBuf", m_outBuf, nLen);
#endif //__ENABLE_DEBUG__

    // Write Buffer Data to hidraw device
    // Since ELAN i2c-hid FW has its special limit, make sure to send all 33 byte once to IC.
    // If data size is not 33, FW will not accept the command even if data format is correct.
    for (nPollIndex = 0; nPollIndex < nPollCount; nPollIndex++)
    {
        nResult = write(m_nHidrawFd, m_outBuf, m_outBufSize);
        if (nResult < 0)
        {
            DBG("%s: Fail to write data! errno=%d.", __func__, nResult);
            nRet = TP_ERR_IO_ERROR;
        }
        else if ((unsigned)nResult != m_outBufSize)
        {
            DBG("%s: Fail to write data! (write_bytes=%d, data_total=%d)", __func__, nResult, m_outBufSize);
            nRet = TP_ERR_IO_ERROR;
        }
        else // Write len bytes of data
        {
            nRet = TP_SUCCESS;
            break;
        }
    }

    // Mutex unlocks the critical section
    sem_post(&m_ioMutex);

WRITE_RAW_BYTES_EXIT:
    return nRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::WriteCommand()
// Write Command Data to HID device
// cBuf: Buffer to write
// nLen: Data length to write
// nTimeout: Time to wait for device respond

int CI2CHIDLinuxGet::WriteCommand(unsigned char* pszCommandBuf, int nCommandLen, int nTimeout, int nDevIdx)
{
    int nRet = TP_SUCCESS;

    // Clear Command Raw Buffer
    memset(m_szOutputBuf, 0, sizeof(m_szOutputBuf));

    // Insert 3-Byte Header Before Command
    if (m_usPID == 0x7)
        m_szOutputBuf[0] = ELAN_HID_OUTPUT_REPORT_ID_PID_B; // HID Report ID
    else
        m_szOutputBuf[0] = ELAN_HID_OUTPUT_REPORT_ID; // HID Report ID
    m_szOutputBuf[1] = 0x0; // Bridge Command
    m_szOutputBuf[2] = nCommandLen; // Command Length

    // Copy 4-Byte / 6-Byte I2C TP Command to Buffer
    memcpy(&m_szOutputBuf[3], pszCommandBuf, nCommandLen);

    // Output Command Raw Buffer
    nRet = WriteRawBytes(m_szOutputBuf, nCommandLen + 3, nTimeout, nDevIdx);
    if (nRet != TP_SUCCESS)
    {
        ERR("%s: Fail to Write Raw Bytes! errno=%d.", __func__, nRet);
    }

    return nRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::ReadRawBytes()
// Read Data from HID device
// cBuf: Buffer to read
// nLen: Data length to read
// nTimeout: Time to wait for device respond
int CI2CHIDLinuxGet::ReadRawBytes(unsigned char* pszBuf, int nLen, int nTimeout, int nDevIdx)
{
    int nRet = TP_SUCCESS,
        nError = 0;

    // Mutex locks the critical section
    sem_wait(&m_ioMutex);

    // Re-initialize file descriptor monitor
    FD_ZERO(&m_fdsHidraw);

    // Add hidraw device handler to file descriptor monitor
    FD_SET(m_nHidrawFd, &m_fdsHidraw);

    // Set wait time up to nTimeout millisecond
    m_tvRead.tv_sec = nTimeout / 1000; // sec
    m_tvRead.tv_usec = (nTimeout % 1000) * 1000; // usec

    // Add file descriptor & timeout to file descriptor monitor
    nError = select(m_nHidrawFd + 1, &m_fdsHidraw, NULL, NULL, &m_tvRead);
    if (nError < 0)
    {
        ERR("%s: File descriptor monitor select fail! errno=%d.", __func__, nError);
        nRet = TP_ERR_IO_ERROR;
        goto READ_RAW_BYTES_EXIT;
    }
    else if (nError == 0)
    {
        DBG("%s: timeout (%d ms)!", __func__, nTimeout);
        nRet = TP_ERR_TIMEOUT; // Timeout error
        goto READ_RAW_BYTES_EXIT;
    }
    else // Read Successfully
    {
        memset(m_inBuf, 0, sizeof(unsigned char)*m_inBufSize);

        if (FD_ISSET(m_nHidrawFd, &m_fdsHidraw))
        {
            nError = read(m_nHidrawFd, m_inBuf, m_inBufSize);
            if (nError < 0)
            {
                ERR("%s: Fail to Read Data! errno=%d.", __func__, nError);
                nRet = TP_ERR_IO_ERROR;
                goto READ_RAW_BYTES_EXIT;
            }

            //DBG("Succesfully read %d bytes.\n", nError);
            nRet = TP_SUCCESS;
        }
    }

    //DBG("Successfully read %d bytes of data from device, return %d.", transfer_cnt, ret);

#ifdef __ENABLE_DEBUG__
    if (g_bEnableDebug)
        DebugPrintBuffer("m_inBuf", m_inBuf, nLen);
#endif //__ENABLE_DEBUG__

    // Copy inBuf data to input buffer pointer
    memcpy(pszBuf, m_inBuf, ((unsigned)nLen <= m_inBufSize) ? nLen : m_inBufSize);

READ_RAW_BYTES_EXIT:
    // Mutex unlocks the critical section
    sem_post(&m_ioMutex);

    return nRet;
}

// Modify by Johnny 20171123
int CI2CHIDLinuxGet::ReadGhostRawBytes(unsigned char* pszBuf, int nLen, int nTimeout, int nDevIdx)
{
    int nRet = TP_SUCCESS,
        nError = 0;

    //DBG("Read start, cBuf=%p, nLen=%d.", cBuf, (int)nLen);

    // Mutex locks the critical section
    sem_wait(&m_ioMutex);

    // Re-initialize file descriptor monitor
    FD_ZERO(&m_fdsHidraw);

    // Add hidraw device handler to file descriptor monitor
    FD_SET(m_nHidrawFd, &m_fdsHidraw);

    // Set wait time up to nTimeout millisecond
    m_tvRead.tv_sec = nTimeout / 1000; // sec
    m_tvRead.tv_usec = (nTimeout % 1000) * 1000; // usec

    // Add file descriptor & timeout to file descriptor monitor
    nError = select(m_nHidrawFd + 1, &m_fdsHidraw, NULL, NULL, &m_tvRead);
    if (nError < 0)
    {
        ERR("%s: File descriptor monitor select fail! errno=%d.", __func__, nError);
        nRet = TP_ERR_IO_ERROR;
        goto READ_RAW_BYTES_EXIT;
    }
    else if (nError == 0)
    {
        ERR("%s: timeout (%d ms)!", __func__, nTimeout);
        nRet = TP_ERR_TIMEOUT; // Timeout error
        goto READ_RAW_BYTES_EXIT;
    }
    else // Read Successfully
    {
        memset(m_inBuf, 0, sizeof(unsigned char)*m_inBufSize);

        if (FD_ISSET(m_nHidrawFd, &m_fdsHidraw))
        {
            nError = read(m_nHidrawFd, m_inBuf, m_inBufSize);
            if (nError < 0)
            {
                ERR("%s: Fail to Read Data! errno=%d.", __func__, nError);
                nRet = TP_ERR_IO_ERROR;
                goto READ_RAW_BYTES_EXIT;
            }

            //DBG("Succesfully read %d bytes.\n", nError);
            nRet = TP_SUCCESS;
        }
    }

    //DBG("Successfully read %d bytes of data from device, return %d.", transfer_cnt, ret);

#ifdef __ENABLE_DEBUG__
    if (g_bEnableDebug)
        DebugPrintBuffer("m_inBuf", m_inBuf, nLen);
#endif //__ENABLE_DEBUG__

    // Copy inBuf data to input buffer pointer
    memcpy(pszBuf, m_inBuf, ((unsigned)nLen <= m_inBufSize) ? nLen : m_inBufSize);

READ_RAW_BYTES_EXIT:
    // Mutex unlocks the critical section
    sem_post(&m_ioMutex);

    return nRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::ReadData()
// Read Data from HID device
// pszDataBuf: Buffer to read
// nDataLen: Data length to read
// nTimeout: Time to wait for device respond

int CI2CHIDLinuxGet::ReadData(unsigned char* pszDataBuf, int nDataLen, int nTimeout, int nDevIdx, bool bFilter)
{
    int nRet = TP_SUCCESS,
        nReportID = 0;

    // Clear Data Raw Buffer
    memset(m_szInputBuf, 0, sizeof(m_szInputBuf));

    // Read 2-Byte Header & Command Data to Data Raw Buffer
    nRet = ReadRawBytes(m_szInputBuf, nDataLen + 2, nTimeout, nDevIdx);
	if (nRet == TP_ERR_TIMEOUT)
	{
		DBG("%s: Fail to Read Raw Bytes! errno=0x%x.", __func__, nRet);
        goto READ_DATA_EXIT;
	}
    else if (nRet != TP_SUCCESS)
    {
        ERR("%s: Fail to Read Raw Bytes! errno=0x%x.", __func__, nRet);
        goto READ_DATA_EXIT;
    }

    // Set Report ID Number for Checking
    if (m_usPID == 0xb)
        nReportID = ELAN_HID_INPUT_REPORT_ID_PID_B; // HID Report ID
    else
        nReportID = ELAN_HID_INPUT_REPORT_ID; // HID Report ID

    // Check if Report ID of Packet is correct
    if ((m_szInputBuf[0] != nReportID) &&
        (m_szInputBuf[0] != ELAN_HID_FINGER_REPORT_ID) &&
        (m_szInputBuf[0] != ELAN_HID_PEN_REPORT_ID)	 &&
        (m_szInputBuf[0] != ELAN_HID_PEN_DEBUG_REPORT_ID))
    {
        nRet = TP_ERR_DATA_PATTERN;
        goto READ_DATA_EXIT;
    }

    if (bFilter == true)
    {
        // Strip 2-Byte Report Header & Load Data to Buffer
        memcpy(pszDataBuf, &m_szInputBuf[2], nDataLen);
    }
    else // if(bFilter == false)
    {
        // Load Report Header & Data to Buffer
        memcpy(pszDataBuf, m_szInputBuf, nDataLen);
    }

READ_DATA_EXIT:
    return nRet;
}

int CI2CHIDLinuxGet::ReadGhostData(unsigned char* pszDataBuf, int nDataLen, int nTimeout, int nDevIdx, bool bFilter)
{
    int nRet = TP_SUCCESS,
        nReportID = 0;

    // Clear Data Raw Buffer
    memset(m_szInputBuf, 0, sizeof(m_szInputBuf));

    // Read 2-Byte Header & Command Data to Data Raw Buffer
    nRet = ReadRawBytes(m_szInputBuf, nDataLen + 2, nTimeout, nDevIdx);
	if (nRet != TP_SUCCESS)
    {
        ERR("%s: Fail to Read Raw Bytes! errno=0x%x.", __func__, nRet);
        goto READ_DATA_EXIT;
    }

    // Set Report ID Number for Checking
    if (m_usPID == 0xb)
        nReportID = ELAN_HID_INPUT_REPORT_ID_PID_B; // HID Report ID
    else
        nReportID = ELAN_HID_INPUT_REPORT_ID; // HID Report ID

    // Check if Report ID of Packet is correct
    if ((m_szInputBuf[0] != nReportID) &&
        (m_szInputBuf[0] != ELAN_HID_FINGER_REPORT_ID) &&
        (m_szInputBuf[0] != ELAN_HID_PEN_REPORT_ID)	 &&
        (m_szInputBuf[0] != ELAN_HID_PEN_DEBUG_REPORT_ID))
    {
        nRet = TP_ERR_DATA_PATTERN;
        goto READ_DATA_EXIT;
    }

    if (bFilter == true)
    {
        // Strip 2-Byte Report Header & Load Data to Buffer
        memcpy(pszDataBuf, &m_szInputBuf[2], nDataLen);
    }
    else // if(bFilter == false)
    {
        // Load Report Header & Data to Buffer
        memcpy(pszDataBuf, m_szInputBuf, nDataLen);
    }

READ_DATA_EXIT:
    return nRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::GetDevVidPid()
// Return Current VID & PID

int	CI2CHIDLinuxGet::GetDevVidPid(unsigned int* p_nVid, unsigned int* p_nPid, int nDevIdx)
{
	int nRet = TP_SUCCESS;

	// Make Sure Input Pointers Valid
	if((p_nVid == NULL) || (p_nPid == NULL))
	{
		ERR("%s: Input Parameters Invalid! (p_nVid=%p, p_nPid=%p)\r\n", __func__, p_nVid, p_nPid);
		nRet = TP_ERR_INVALID_PARAM;
		goto GET_DEV_VID_PID_EXIT;
	}

	// Make Sure Device Found
	if((m_usVID == 0) && (m_usPID == 0))
	{
		ERR("%s: I2C-HID device has never been found!\r\n", __func__);
		nRet = TP_ERR_NOT_FOUND_DEVICE;
		goto GET_DEV_VID_PID_EXIT;
	}

	// Set PID & VID
	*p_nVid = m_usVID;
	*p_nPid = m_usPID;

	// Success
	nRet = TP_SUCCESS;

GET_DEV_VID_PID_EXIT:
	return nRet;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::GetInBufferSize()
// Return Input Buffer Size

int CI2CHIDLinuxGet::GetInBufferSize(void)
{
    //DBG("Current Input Buffer Size = %d.", m_inBufSize);
    return m_inBufSize;
}

/////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::GetOutBufferSize()
// Return Output Buffer Size

int CI2CHIDLinuxGet::GetOutBufferSize(void)
{
    //DBG("Current Output Buffer Size = %d.", m_outBufSize);
    return m_outBufSize;
}

////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::GetInterfaceType()
// Return Interface Type

int CI2CHIDLinuxGet::GetInterfaceType(void)
{
    return INTF_TYPE_I2CHID_LINUX;
}

////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::GetInterfaceVersion()
// Return Version of Interface Inplementation

const char* CI2CHIDLinuxGet::GetInterfaceVersion(void)
{
    return I2CHID_LINUX_INTF_IMPL_VER;
}

////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::bus_str()
// Get Bus String from BUS ID
const char* CI2CHIDLinuxGet::bus_str(int bus)
{
    switch (bus)
    {
        case BUS_USB:
            return "USB";

        case BUS_HIL:
            return "HIL";

        case BUS_BLUETOOTH:
            return "Bluetooth";

#if 0 // Disable this convert since the definition is not include in input.h of android sdk.
        case BUS_VIRTUAL:
            return "Virtual";
#endif // 0

        case BUS_I2C:
            return "I2C";

        default:
            return "Other";
    }
}

////////////////////////////////////////////////////////////////////////////
// CI2CHIDLinuxGet::FindHidrawDevice()
// Find hidraw device name with specific VID and PID, such as /dev/hidraw0
int CI2CHIDLinuxGet::FindHidrawDevice(int nVID, int nPID, char *pszDevicePath)
{
    int nRet = TP_SUCCESS,
        nError = 0,
        nFd = 0;
    bool bFound = false;
    DIR *pDirectory = NULL;
    struct dirent *pDirEntry = NULL;
    const char *pszPath = "/dev";
    char szFile[64] = {0};
    struct hidraw_devinfo info;

    // Check if filename ptr is valid
    if (pszDevicePath == NULL)
    {
        ERR("%s: NULL Device Path Buffer!", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto FIND_ELAN_HIDRAW_DEVICE_EXIT;
    }

    // Open Directory
    pDirectory = opendir(pszPath);
    if (pDirectory == NULL)
    {
        ERR("%s: Fail to Open Directory %s.\r\n", __func__, pszPath);
        nRet = TP_ERR_NOT_FOUND_DEVICE;
        goto FIND_ELAN_HIDRAW_DEVICE_EXIT;
    }

    // Traverse Directory Elements
    while ((pDirEntry = readdir(pDirectory)) != NULL)
    {
        // Only reserve hidraw devices
        if (strncmp(pDirEntry->d_name, "hidraw", 6))
            continue;

        memset(szFile, 0, sizeof(szFile));
        sprintf(szFile, "%s/%s", pszPath, pDirEntry->d_name);
        DBG("%s: file=\"%s\".", __func__, szFile);

        /* Open the Device with non-blocking reads. In real life,
          don't use a hard coded path; use libudev instead. */
        nError = open(szFile, O_RDWR | O_NONBLOCK);
        if (nError < 0)
        {
            DBG("%s: Fail to Open Device %s! errno=%d.", pDirEntry->d_name, nError);
            continue;
        }
        nFd = nError;

        /* Get Raw Info */
        nError = ioctl(nFd, HIDIOCGRAWINFO, &info);
        if (nError >= 0)
        {
            DBG("--------------------------------");
            DBG("\tbustype: 0x%02x (%s)", info.bustype, bus_str(info.bustype));
            DBG("\tvendor: 0x%04hx", info.vendor);
            DBG("\tproduct: 0x%04hx", info.product);

            // Force touch device to connect if bustype=0x03(BUS_I2C), VID=0x4f3, and PID=0x0
            if ((info.bustype == BUS_I2C) && 
				(info.vendor == ELAN_USB_VID) /* nVID = usb_dev_desc.idVendor = ELAN_USB_VID */  &&
                (nPID == ELAN_USB_FORCE_CONNECT_PID))
            {
                // Use found PID from Hid-Raw
                nPID = info.product;
                DBG("%s: bustype=0x%02x, VID=0x%04x, PID=0x%04x => PID changes to 0x%04x.", __func__, BUS_I2C, ELAN_USB_VID, ELAN_USB_FORCE_CONNECT_PID, nPID);
            }

            if ((info.vendor == nVID) && (info.product == nPID))
            {
                DBG("%s: Found hidraw device with VID 0x%x and PID 0x%x!", __func__, nVID, nPID);
				m_usVID = (unsigned short) nVID;
				m_usPID = (unsigned short) nPID;
                memcpy(pszDevicePath, szFile, sizeof(szFile));
                bFound = true;
            }
        }

        // Close Device
        close(nFd);

        // Stop the loop if found
        if (bFound == true)
            break;
    }

    if (!bFound)
        nRet = TP_ERR_NOT_FOUND_DEVICE;

    // Close Directory
    closedir(pDirectory);

FIND_ELAN_HIDRAW_DEVICE_EXIT:
    return nRet;
}
