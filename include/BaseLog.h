// BaseLog.h: Declaration for the CBaseLog class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BASELOG_H__
#define __BASELOG_H__
#ifdef _WIN32
#pragma warning(disable:4996) // Disable warning C4996: XXX: This function of variable may be unsafe.
#endif //_WIN32

#include <stdio.h>
#include <string.h>
#include <time.h> //<ctime>

#ifdef __linux__
#include <sys/time.h> // struct timeval & gettimeofday()
#include <semaphore.h>	/* semaphore */
#include <syslog.h>     /* syslog */

#else // _WIN32
#include <sys_win32/time.h> // struct timeval & gettimeofday()
#include <windows.h> /* CRITICAL_SECTION */
#endif //__linux__

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

#ifndef __ENABLE_SYSLOG_DEBUG__
#define __ENABLE_SYSLOG_DEBUG__
#endif //__ENABLE_SYSLOG_DEBUG__

#if 0 //ndef __ENABLE_LOG_FILE_DEBUG__
#define __ENABLE_LOG_FILE_DEBUG__
#endif //__ENABLE_LOG_FILE_DEBUG__

#ifdef __ENABLE_SYSLOG_DEBUG__
// const char LOG_DIR[] = ".";
#ifndef DEFAULT_DEBUG_LOG_DIR
#define DEFAULT_DEBUG_LOG_DIR	"/tmp"
#endif //DEFAULT_DEBUG_LOG_FILE

//const char LOG_FILE[] = "log.txt";
#ifndef DEFAULT_DEBUG_LOG_FILE
#define DEFAULT_DEBUG_LOG_FILE	"elan_i2chid_iap_log.txt"
#endif // DEFAULT_DEBUG_LOG_FILE

//const char PRODUCTIONTESTDATA_LOG[]		= "ProductionTestData.csv";
#ifndef DEFAULT_TEST_RESULT_LOG_FILE
#define DEFAULT_TEST_RESULT_LOG_FILE	"ProductionTestData.csv"
#endif // DEFAULT_TEST_RESULT_LOG_FILE
#endif //__ENABLE_SYSLOG_DEBUG__

//const int LOG_BUF_SIZE = 1280;
#ifndef LOG_BUF_SIZE
#define LOG_BUF_SIZE 4096
#endif //LOG_BUF_SIZE

#ifndef PATH_LEN_MAX
#define PATH_LEN_MAX 512
#endif //LOG_BUF_SIZE

//////////////////////////////////////////////////////////////////////
// Global Variable
//////////////////////////////////////////////////////////////////////

extern bool g_bEnableDebug;
extern bool g_bEnableOutputBufferDebug;
extern bool g_bEnableErrorMsg;

//////////////////////////////////////////////////////////////////////
// Macro
//////////////////////////////////////////////////////////////////////

#ifdef __linux__
#define DEBUG(format, args...) if(g_bEnableDebug)    DebugLogFormat(format, ##args)
#define   DBG(format, args...) if(g_bEnableDebug)    DebugLogFormat(format, ##args)

#define ERROR(format, args...) \
do{\
   printf("[ERROR] " format "\r\n", ##args); \
   if(g_bEnableErrorMsg) \
      ErrorLogFormat(format, ##args); \
}while(0)

#define ERR(format, args...) \
do{\
   printf("[ERR] " format "\r\n", ##args); \
   if(g_bEnableErrorMsg) \
      ErrorLogFormat(format, ##args); \
}while(0)

#define INFO(format, args...) \
do{\
   printf("[INFO] " format "\r\n", ##args); \
   if(g_bEnableDebug) \
      DebugLogFormat(format, ##args); \
}while(0)
#else // _WIN32
#define DEBUG(format, ...) if(g_bEnableDebug) DebugLogFormat(format, __VA_ARGS__)
#define   DBG(format, ...) if(g_bEnableDebug) DebugLogFormat(format, __VA_ARGS__)

/* warning C4005: 'ERROR' macro redefinition. (Win10 SDK: wingdi.h)
#define ERROR(format, ...) \
do{\
   printf("[ERROR] "format"\r\n", __VA_ARGS__); \
   if(g_bEnableErrorMsg) \
      ErrorLogFormat(format, __VA_ARGS__); \
}while(0)
*/

#define ERR(format, ...) \
do{\
   printf("[ERROR] " format "\r\n", __VA_ARGS__); \
   if(g_bEnableErrorMsg) \
      ErrorLogFormat(format, __VA_ARGS__); \
}while(0)

#define INFO(format, ...) \
do{\
   printf("[INFO] " format "\r\n", __VA_ARGS__); \
   if(g_bEnableDebug) \
      DebugLogFormat(format, __VA_ARGS__); \
}while(0)
#endif // __linux__

//////////////////////////////////////////////////////////////////////
// Prototype
//////////////////////////////////////////////////////////////////////

class CBaseLog
{
public:
    // Constructor / Deconstructor
    //CBaseLog(void);
    CBaseLog(char *pszLogDirPath = (char *)DEFAULT_DEBUG_LOG_DIR, char *pszDebugLogFileName = (char *)DEFAULT_DEBUG_LOG_FILE);

    ~CBaseLog(void);

    // Log Functions
    void DebugLog(char *pszLog);
    void ErrorLog(char *pszLog);
    void DebugLogFormat(const char *pszFormat, ...);
    void ErrorLogFormat(const char *pszFormat, ...);
    void DebugPrintBuffer(unsigned char *pbyBuf, int nLen);
    void DebugPrintBuffer(const char *pszBufName, unsigned char *pbyBuf, int nLen);

#ifdef __ENABLE_LOG_FILE_DEBUG__
    // File Operation
    int CleanFileContentWithPath(const char *pszFilePath);
    int CopyFileA(const char *pszSrcFilePath, char *pszDestFilePath);

    // Path-Related Functions
    int GetDirPath(char *pszFullPath, char *pszDirPath);
    int GetFileName(char *pszFullPath, char *pszFileName);
    int CreateDirPath(char *pszDirPath);
    int SetLogDirPath(char *pszDirPath);
    int GetLogDirPath(char *pszDirPathBuffer);

    int SetDebugLogFileName(char *pszFileName);
    int GetDebugLogFileName(char *pszFileNameBuffer);
    int GetDebugLogFilePath(char *pszFilePathBuffer);

    int SetTestResultLogFileName(char *pszFileName);
    int GetTestResultLogFileName(char *pszFileNameBuffer);
    int GetTestResultLogFilePath(char *pszFilePathBuffer);

    // Data Member
    time_t m_tRawTime;
    struct timeval m_tvCurTime;
    char m_szDateBuffer[80];
    char m_szDateTimeBuffer[88];

    // Debug Log
    char m_szLogDirPath[PATH_LEN_MAX];
    char m_szDebugLogFileName[PATH_LEN_MAX];
    char m_szDebugLogFilePath[PATH_LEN_MAX];

    // Test Result log
    char m_szTestResultLogFileName[PATH_LEN_MAX];
    char m_szTestResultLogFilePath[PATH_LEN_MAX];
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Data Member
    char m_szLogBuf[LOG_BUF_SIZE];
    char m_szDebugBuf[LOG_BUF_SIZE];

protected:
#ifdef __linux__
    sem_t m_semFileIoMutex;
#else // _WIN32
    CRITICAL_SECTION m_csFileIoMutex;
#endif //__linux__
    int m_nFileIoLockCounter;
}; //CBaseLog

#endif //ndef __BASELOG_H__
