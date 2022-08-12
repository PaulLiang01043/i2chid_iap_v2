#ifndef __INTERFACEGET_H__
#define __INTERFACEGET_H__
#pragma once
#ifdef _WIN32
#pragma warning(disable:4996) // Disable warning C4996: XXX: This function of variable may be unsafe.
#endif //_WIN32

/**************************************************************************
* IntefaceGet.h
* Base Class of CHIDGet.h, CLinuxHIDGet.h, and CLinuxI2CGet.h.
* This class contains of 8 virtual methods needed to be implemented:
*
*	int GetDeviceHandle(void);
*	int GetDeviceHandle(int nVID, int nPID);
*	void Close(void);
*	bool IsConnected(void);
*
*	int WriteData(unsigned char* cBuf, size_t nLen);
*	int ReadData(unsigned char* cBuf, size_t nLen);
*	int ReadData(unsigned char* cBuf, size_t nLen, int nTimeout);
*
*	int GetVersion(void);
*   int GetPID(void);
*
* Date: 2013/06/26
**************************************************************************/
#include <cstdio>
#include "BuildConfig.h"
#include "ErrCode.h"

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

// Interface Type
const int INTF_TYPE_NONE						= 0;
const int INTF_TYPE_HID_WINDOWS					= 1;
const int INTF_TYPE_HID_LINUX					= 2;
const int INTF_TYPE_I2C_LINUX					= 3;
const int INTF_TYPE_I2CHID_LINUX				= 4;
const int INTF_TYPE_I2C_CHROME_LINUX			= 5;
const int INTF_TYPE_ELAN_I2CHID_LINUX			= 6;
const int INTF_TYPE_SOCKET						= 7;
const int INTF_TYPE_ELAN_I2CHID_CHROME_LINUX	= 8;

// General Data Length Setting
const int MAX_LENGTH = 256;

// Timeout Setting
const int ELAN_POLL_DATA_TIMEOUT_MSEC  = 1;
const int ELAN_READ_DATA_TIMEOUT_MSEC  = 1000;
const int ELAN_WRITE_DATA_TIMEOUT_MSEC = 1000;

// EEPROM Access Type
const int EEPROM_ACCESS_TYPE_WRITE		= 0;
const int EEPROM_ACCESS_TYPE_READ		= 1;

// Max Power Configuration Item Number
#ifndef MAX_POWER_CONFIG_ITEM
#define MAX_POWER_CONFIG_ITEM	4
#endif //MAX_POWER_CONFIG_ITEM

//////////////////////////////////////////////////////////////////////
// Declaration of Data Structure
//////////////////////////////////////////////////////////////////////

// Power Configuration
struct power_configurartion
{
    char szTool[MAX_PATH];
    char szCommand[MAX_PATH];
    char szAddress[MAX_POWER_CONFIG_ITEM][MAX_PATH];
    char szValue[MAX_POWER_CONFIG_ITEM][MAX_PATH];
    int nCommandDelayMSec;
};
typedef struct power_configurartion structPowerConfig;

//////////////////////////////////////////////////////////////////////
// Prototype
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
//#define WINAPI      __stdcall // WIN32 Definition of "__stdcall"
//J2++
typedef void (__stdcall *PFUNC_OUT_REPORT_CALLBACK)(unsigned char* pReportBuf, int nReportLen);
typedef void (__stdcall *PFUNC_IN_REPORT_CALLBACK)(unsigned char* pReportBuf, int nReportLen);
typedef void (__stdcall *PFUNC_SOCKET_EVENT_CALLBACK)(int nEvetID);
//J2--
#endif //_WIN32

class CInterfaceGet
{
public:
    // Constructor / Deconstructor
    CInterfaceGet(void) {};
    virtual ~CInterfaceGet(void) {};

    // Interface Info.
    virtual int GetInterfaceType(void) = 0;						// Must Implement
    virtual const char* GetInterfaceVersion(void) = 0;			// Must Implement

    // Basic Functions
    //virtual int GetDeviceHandle(void) = 0; // No Longer In Use
    /*******************************************************
     * GetDeviceHandle: Connect Device & Get Handle
     * nParam1: DrvInterface when I2C, VID when HID/I2CHID
     * nParam2: BusID when I2C, PID when HID/I2CHID
     *******************************************************/
    virtual int GetDeviceHandle(int nParam1, int nParam2) = 0;
    virtual void Close(void) = 0;
    virtual bool IsConnected(void) = 0;

    //
    // TP Command / Data Access Functions in Main Code
    //
    // pszCommandBuf only contains the command data which firmware can recognize.
    // Report ID or brige command is not includes.
    virtual int WriteCommand(unsigned char* pszCommandBuf, int nBufLen, int nTimeoutMS, int nDevIdx) = 0;
    virtual int ReadData(unsigned char* pszDataBuf, int nBufLen, int nTimeoutMS, int nDevIdx, bool bFilter) = 0;

    // Modify by Johnny 20171123
    virtual int ReadGhostData(unsigned char* pszDataBuf, int nBufLen, int nTimeoutMS, int nDevIdx, bool bFilter) = 0;

    //
    // Raw Data Access Functions
    // Read and Write RAW data support multiple devices
    //
    virtual int WriteRawBytes(unsigned char* pszBuf, int nBufLen, int nTimeoutMS, int nDevIdx) = 0;
    virtual int ReadRawBytes(unsigned char* pszBuf, int nBufLen, int nTimeoutMS, int nDevIdx) = 0;

    // Modify by Johnny 20171123
    virtual int ReadGhostRawBytes(unsigned char* pszBuf, int nBufLen, int nTimeoutMS, int nDevIdx) = 0;

    // Buffer Size Info.
    virtual int GetInBufferSize(void) { return 0; }
    virtual int GetOutBufferSize(void) { return 0; }

    //
    // For multiple device on one pc, nDevIdx will specify the target device.
    // Only windows platform support the following function.
    //
    virtual int GetDevCount(void) { return 1; }
    virtual int GetDevProfile(void* pProfile, int nDevIdx) { return TP_ERR_COMMAND_NOT_SUPPORT; }
    virtual int SetDevProfile(void* pProfile, int nDevIdx) { return TP_ERR_COMMAND_NOT_SUPPORT; }
    virtual int GetDevParsedReport(unsigned char* pData, int iDataLen, void* pReport, int nDevIdx) { return TP_ERR_COMMAND_NOT_SUPPORT; }

    // Alan 20161129
    virtual int GetDevPairNum(int nDevIdx, char *pszDevPairNum, int strlen) { return TP_ERR_COMMAND_NOT_SUPPORT; }
    virtual int GetDevPath(int nDevIdx, char *pszDevPath, int strlen)		{ return TP_ERR_COMMAND_NOT_SUPPORT; }
    virtual int GetIcDevType(int nDevIdx, char *pszIcDevType, int strlen)	{ return TP_ERR_COMMAND_NOT_SUPPORT; }
    virtual int GetDevVersion(unsigned int* p_nVer, int nDevIdx)			{ return TP_ERR_COMMAND_NOT_SUPPORT; }
    //~Alan 20161129

    virtual int GetDevVidPid(unsigned int* p_nVid, unsigned int* p_nPid, int nDevIdx) { return TP_ERR_COMMAND_NOT_SUPPORT; }

    // option. Only for Linux/I2C
    virtual int SwitchChip(int nChipID)			{ return TP_ERR_COMMAND_NOT_SUPPORT; }
    virtual int EnableDriverIRQ(bool bEnable)	{ return TP_ERR_COMMAND_NOT_SUPPORT; }

    // option. Only for Linux/I2CChrome
    virtual int SetPower(structPowerConfig *pPowerConfig) { return TP_ERR_COMMAND_NOT_SUPPORT; }

    // option. Only for windows.
    virtual int SetDeviceReportID(int nInputReportID, int nOutputReportID) { return TP_ERR_COMMAND_NOT_SUPPORT; }

    // EEPROM Function (Provided by I2C-HID Bridge & Only Avaliable in I2C-HID Windows Environment)
    virtual int WriteEEPROMCommand(int nWrRdType, unsigned char* pszDataBuf, int nSize, int nTimeoutMS, int nDevIdx) { return TP_ERR_COMMAND_NOT_SUPPORT; }

#ifdef _WIN32
    //J2++
    void SetOutReportFuncPtr(PFUNC_OUT_REPORT_CALLBACK pFunc)		{ m_pOutReportFuncPtr	= pFunc; }
    void SetInReportFuncPtr(PFUNC_IN_REPORT_CALLBACK pFunc)			{ m_pInReportFuncPtr	= pFunc; }
    void SetSocketEventFuncPtr(PFUNC_SOCKET_EVENT_CALLBACK pFunc)	{ m_pSocketEventFuncPtr = pFunc; }
    //J2--
#endif //_WIN32

protected:

#ifdef _WIN32
    //J2++
    //Callback function pointer
    PFUNC_OUT_REPORT_CALLBACK	m_pOutReportFuncPtr;
    PFUNC_IN_REPORT_CALLBACK	m_pInReportFuncPtr;
    PFUNC_SOCKET_EVENT_CALLBACK m_pSocketEventFuncPtr;
    //J2--
#endif //_WIN32

};
#endif //__INTERFACEGET_H__
