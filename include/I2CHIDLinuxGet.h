// I2CHIDLinuxGet.h: Declaration for the CI2CHIDLinuxGet class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __I2CHIDLINUXGET_H__
#define __I2CHIDLINUXGET_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <semaphore.h>			/* semaphore */
#include <sys/select.h>         /* select */
#include <sys/time.h>           /* timeval */
#include <errno.h>              /* errno */
#include "InterfaceGet.h"
#include "BaseLog.h"

//////////////////////////////////////////////////////////////////////
// Version of Interface Implementation
//////////////////////////////////////////////////////////////////////
#ifndef I2CHID_LINUX_INTF_IMPL_VER
#define I2CHID_LINUX_INTF_IMPL_VER	"I2CHIDLinuxGet Version : 0.0.0.1"
#endif //I2CHID_LINUX_INTF_IMPL_VER

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

// ELAN HID Settings
const int ELAN_HID_TRANSFER_INPUT_TIMEOUT = 2000; //2s //1000; //1s //400; //400ms //300; //300ms //500; //500ms //1000; //1s //100; //100ms //10; //10ms //10000;
const int ELAN_HID_CONNECT_RETRY = 20;

/* 		ELAN Output Buffer Format 			*
 *								*
 *    | Report ID (0x03, 1-byte) | Output Report (32-byte) |	*
 *								*
 *              ELAN Input Buffer Format                        *
 *                                                              *
 *    | Report ID (0x02, 1-byte) | Input Report (64-byte)  |	*
 */

/* ELAN I2C-HID Buffer Size */
const int ELAN_I2CHID_OUTPUT_BUFFER_SIZE = 0x21; //1+32
const int ELAN_I2CHID_INPUT_BUFFER_SIZE  = 0x41; //1+64

#ifndef __ELAN_HID_DEFINITION__
// ELAN Default VID
#ifndef ELAN_USB_VID
#define ELAN_USB_VID					0x04F3
#endif //ELAN_USB_VID

// ELAN Recovery PID
#ifndef ELAN_USB_RECOVERY_PID
#define ELAN_USB_RECOVERY_PID			0x0732
#endif //ELAN_USB_RECOVERY_PID

// PID 0x0: Connect whatever elan device found.
#ifndef ELAN_USB_FORCE_CONNECT_PID
#define ELAN_USB_FORCE_CONNECT_PID		0x0
#endif //ELAN_USB_FORCE_CONNECT_PID

// ELAN HID Report ID
//const int ELAN_HID_OUTPUT_REPORT_ID	= 0x3;
#ifndef ELAN_HID_OUTPUT_REPORT_ID
#define ELAN_HID_OUTPUT_REPORT_ID		0x3
#endif //ELAN_HID_OUTPUT_REPORT_ID

//const int ELAN_HID_INPUT_REPORT_ID	= 0x2;
#ifndef ELAN_HID_INPUT_REPORT_ID
#define ELAN_HID_INPUT_REPORT_ID		0x2
#endif //ELAN_HID_INPUT_REPORT_ID

//const int ELAN_HID_FINGER_REPORT_ID	= 0x1;
#ifndef ELAN_HID_FINGER_REPORT_ID
#define ELAN_HID_FINGER_REPORT_ID		0x1
#endif //ELAN_HID_FINGER_REPORT_ID

#ifndef ELAN_HID_PEN_REPORT_ID
#define ELAN_HID_PEN_REPORT_ID			0x7
#endif //ELAN_HID_PEN_REPORT_ID	

#ifndef ELAN_HID_PEN_DEBUG_REPORT_ID
#define ELAN_HID_PEN_DEBUG_REPORT_ID	0x17
#endif //ELAN_HID_PEN_DEBUG_REPORT_ID

// For I2C FW with M680 Bridge (PID 0xb)
const int ELAN_HID_OUTPUT_REPORT_ID_PID_B	= 0x0;
const int ELAN_HID_INPUT_REPORT_ID_PID_B	= 0x0;

// For I2C/I2CHID FW with Universal Bridge (PID 0x7)
const int ELAN_HID_OUTPUT_REPORT_ID_PID_7	= 0x0;
const int ELAN_HID_INPUT_REPORT_ID_PID_7	= 0x0;
#define __ELAN_HID_DEFINITION__
#endif //__ELAN_HID_DEFINITION__

/////////////////////////////////////////////////////////////////////////////
// CHIDGet Class

class CI2CHIDLinuxGet: public CInterfaceGet, public CBaseLog
{
public:
    // Constructor / Deconstructor
    CI2CHIDLinuxGet(char *pszLogDirPath = (char *)DEFAULT_DEBUG_LOG_DIR, char *pszDebugLogFileName = (char *)DEFAULT_DEBUG_LOG_FILE);
    ~CI2CHIDLinuxGet(void);

    // Interface Info.
    int GetInterfaceType(void);
    const char* GetInterfaceVersion(void);

    // Basic Functions
    int GetDeviceHandle(int nVID, int nPID);
    void Close(void);
    bool IsConnected(void);

    // TP Command / Data Access Functions
    int WriteCommand(unsigned char* pszCommandBuf, int nCommandLen, int nTimeout = ELAN_WRITE_DATA_TIMEOUT_MSEC, int nDevIdx = 0);
    int ReadData(unsigned char* pszDataBuf, int nDataLen, int nTimeout = ELAN_READ_DATA_TIMEOUT_MSEC, int nDevIdx = 0, bool bFilter = true);

    // Modify by Johnny 20171123
    int ReadGhostData(unsigned char* pszDataBuf, int nDataLen, int nTimeout = ELAN_READ_DATA_TIMEOUT_MSEC, int nDevIdx = 0, bool bFilter = true);

    // Raw Data Access Functions
    int WriteRawBytes(unsigned char* pszBuf, int nLen, int nTimeout = ELAN_WRITE_DATA_TIMEOUT_MSEC, int nDevIdx = 0);
    int ReadRawBytes(unsigned char* pszBuf, int nLen, int nTimeout = ELAN_READ_DATA_TIMEOUT_MSEC, int nDevIdx = 0);

    // Modify by Johnny 20171123
    int ReadGhostRawBytes(unsigned char* pszBuf, int nLen, int nTimeout = ELAN_READ_DATA_TIMEOUT_MSEC, int nDevIdx = 0);

    // Buffer Size Info.
    int GetInBufferSize(void);
    int GetOutBufferSize(void);

    // PID
    int	GetDevVidPid(unsigned int* p_nVid, unsigned int* p_nPid, int nDevIdx = 0);

protected:
    // Basic Functions

    const char* bus_str(int bus);
    int FindHidrawDevice(int nVID, int nPID, char *pszDevicePath);

    int m_nHidrawFd;
    fd_set m_fdsHidraw;
    struct timeval m_tvRead;

    unsigned char *m_inBuf;
    unsigned char *m_outBuf;
    unsigned int m_inBufSize;
    unsigned int m_outBufSize;
    sem_t m_ioMutex;

    unsigned short m_usVID;	// Vendor ID
    unsigned short m_usPID;	// Product ID
    unsigned short m_usVersion;	// HID Version

    unsigned char m_szOutputBuf[32 /* ELAN_USB_OUTPUT_LEN */];    // Command Raw Buffer
    unsigned char m_szInputBuf[128 /* ELAN_USB_INPUT_LEN * 2 */]; // Data Raw Buffer
};
#endif //__I2CHIDLINUXGET_H__
