// Log.cpp: implementation of the CLog class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h" // Comment out this line if not windows
#include <stdio.h>
#include <stdarg.h>
#ifdef __linux__
#include <unistd.h>
#else
#include <io.h>
#endif //__linux__
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include "BaseLog.h"
#include "ErrCode.h" // Error Code

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#ifndef S_ISDIR
#define S_ISDIR(mode)	(((mode) & _S_IFDIR) == _S_IFDIR)
#endif //S_ISDIR
#endif //_WIN32

//////////////////////////////////////////////////////////////////////
// Global Variable
//////////////////////////////////////////////////////////////////////

// Debug
bool g_bEnableDebug = true;
bool g_bEnableOutputBufferDebug = true;
bool g_bEnableErrorMsg = true;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBaseLog::CBaseLog(char *pszLogDirPath, char *pszDebugLogFileName)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    struct stat file_stat;
    //printf("%s: pszLogDirPath=\"%s\", pszDebugLogFileName=\"%s\".\r\n", __func__, pszLogDirPath, pszDebugLogFileName);
#endif //__ENABLE_LOG_FILE_DEBUG__

    memset(m_szLogBuf, 0, sizeof(m_szLogBuf));
    memset(m_szDebugBuf, 0, sizeof(m_szDebugBuf));

#ifdef __ENABLE_LOG_FILE_DEBUG__
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    memset(m_szLogDirPath, 0, sizeof(m_szLogDirPath));
    memset(m_szDebugLogFileName, 0, sizeof(m_szDebugLogFileName));
    memset(m_szDebugLogFilePath, 0, sizeof(m_szDebugLogFilePath));
    memset(m_szTestResultLogFileName, 0, sizeof(m_szTestResultLogFileName));
    memset(m_szTestResultLogFilePath, 0, sizeof(m_szTestResultLogFilePath));
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Initialize mutex
#ifdef __linux__
    sem_init(&m_semFileIoMutex,  0 /*scope is in this file*/, 1 /*active in initial*/);
#else // _WIN32
    InitializeCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter = 0;

#if defined(__linux__) && defined(__ENABLE_SYSLOG_DEBUG__)
    // syslog
    openlog("elan_i2chid_debug", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
#endif //defined(__linux__) && defined(__ENABLE_SYSLOG_DEBUG__)

#ifdef __ENABLE_LOG_FILE_DEBUG__
    // Initialize Debug Directory Path
    if (pszLogDirPath != NULL)
        SetLogDirPath(pszLogDirPath);

    /* Initialize Debug Log File Name & Path */
    if ((pszDebugLogFileName == NULL) || (strcmp(pszDebugLogFileName, "") == 0))
    {
        // Initialize Debug Log File Name
        strcpy(m_szDebugLogFileName, DEFAULT_DEBUG_LOG_FILE);

        // Update File Path
        if (strcmp(m_szLogDirPath, "") == 0) // Log Dir Path is Null
            sprintf(m_szDebugLogFilePath, "%s", m_szDebugLogFileName);
        else
#ifdef __linux__
            sprintf(m_szDebugLogFilePath, "%s/%s", m_szLogDirPath, m_szDebugLogFileName);
#else //_WIN32
            sprintf(m_szDebugLogFilePath, "%s\\%s", m_szLogDirPath, m_szDebugLogFileName);
#endif //__linux__

        // Clear Content of Debug Log File
        if (stat(m_szDebugLogFilePath, &file_stat) == 0)
        {
            //CleanFileContentWithPath(DEFAULT_DEBUG_LOG_FILE);
            remove(m_szDebugLogFilePath);
        }
    }
    else // (pszDebugLogFileName != NULL) && (strcmp(pszDebugLogFileName, "") != 0)
    {
        SetDebugLogFileName(pszDebugLogFileName);
    }
    //printf("%s: DebugLogFileName=\"%s\", DebugLogFilePath=\"%s\".\r\n", __func__, m_szDebugLogFileName, m_szDebugLogFilePath);

    /* Initialize Test Result Log File Name & Path */

    // Initialize Test Result Log File Name
    strcpy(m_szTestResultLogFileName, DEFAULT_TEST_RESULT_LOG_FILE);

    // Update File Path
    if (strcmp(m_szLogDirPath, "") == 0) // Log Dir Path is Null
        sprintf(m_szTestResultLogFilePath, "%s", m_szTestResultLogFileName);
    else
#ifdef __linux__
        sprintf(m_szTestResultLogFilePath, "%s/%s", m_szLogDirPath, m_szTestResultLogFileName);
#else //_WIN32
        sprintf(m_szTestResultLogFilePath, "%s\\%s", m_szLogDirPath, m_szTestResultLogFileName);
#endif //__linux__
    //printf("%s: TestResultLogFileName=\"%s\", TestResultLogFilePath=\"%s\".\r\n", __func__, m_szTestResultLogFileName, m_szTestResultLogFilePath);

    // Clear Content of Debug Log File
    if (stat(m_szTestResultLogFilePath, &file_stat) == 0)
        remove(m_szTestResultLogFilePath);
#endif //__ENABLE_LOG_FILE_DEBUG__
}

CBaseLog::~CBaseLog(void)
{
#if defined(__linux__) && defined(__ENABLE_SYSLOG_DEBUG__)
    // syslog
    closelog();
#endif //defined(__linux__) && defined(__ENABLE_SYSLOG_DEBUG__)

    // Destroy mutex (semaphore)
    //DBG("Destroy mutext/semaphore (address=%p).", &m_semFileIoMutex);
#ifdef __linux__
    sem_destroy(&m_semFileIoMutex);
#else // _WIN32
    DeleteCriticalSection(&m_csFileIoMutex);
#endif // __linux__
}

//////////////////////////////////////////////////////////////////////
// Function Implementation
//////////////////////////////////////////////////////////////////////

#ifdef __ENABLE_LOG_FILE_DEBUG__
int CBaseLog::GetDirPath(char *pszFullPath, char *pszDirPath)
{
    int nRet = TP_SUCCESS,
        nSubPathIndex = 0;
    char szFullPath[PATH_LEN_MAX] = {0},
                                    *pszSubPath = NULL,
                                     *pcSlash = NULL;

    // Make Sure Input Pointers are Valid
    if ((pszFullPath == NULL) || (pszDirPath == NULL))
    {
        printf("%s: NULL Input Pointer! (pszFullPath=%p, pszDirPath=%p)\r\n",
               __func__, pszFullPath, pszDirPath);
        nRet = TP_ERR_INVALID_PARAM;
        goto GET_DIR_PATH_EXIT;
    }

    // Copy Full Path to local Path Buffer
    strcpy(szFullPath, pszFullPath);

    // Find The Last Slash
    pszSubPath = &szFullPath[1];
    //printf("%s: Sub Path %d: %s.\r\n", __func__, nSubPathIndex, pszSubPath);
    while ((pcSlash = strpbrk(pszSubPath, "\\/")) != NULL)
    {
        pszSubPath = pcSlash + 1;
        nSubPathIndex++;
        printf("%s: Sub Path %d: %s.\r\n", __func__, nSubPathIndex, pszSubPath);
    }

    // Make Sure Slash Had Ever Been Found
    if (pszSubPath == &szFullPath[1])
    {
        printf("%s: No Directory In Path!\r\n", __func__);
        strcpy(pszDirPath, pszFullPath);
        nRet = TP_ERR_FILE_NOT_FOUND;
        goto GET_DIR_PATH_EXIT;
    }

    // Change The Full Path to Directory Path
    *(pszSubPath - 1) = '\0'; // Replace the Last Slash With Null Termination Character '\0'.
    //printf("%s: Dir Path of Path \"%s\" is \"%s\".\r\n", __func__, pszFullPath, szFullPath);
    strcpy(pszDirPath, szFullPath);

GET_DIR_PATH_EXIT:
    return nRet;
}

int CBaseLog::GetFileName(char *pszFullPath, char *pszFileName)
{
    int nRet = TP_SUCCESS,
        nSubPathIndex = 0;
    char *pszSubPath = NULL,
          *pcSlash = NULL;

    // Make Sure Input Pointers are Valid
    if ((pszFullPath == NULL) || (pszFileName == NULL))
    {
        printf("%s: NULL Input Pointer! (pszFullPath=%p, pszFileName=%p)\r\n",
               __func__, pszFullPath, pszFileName);
        nRet = TP_ERR_INVALID_PARAM;
        goto GET_FILE_NAME_EXIT;
    }

    // Find The Last Slash
    pszSubPath = pszFullPath + 1;
    //printf("%s: Sub Path %d: %s.\r\n", __func__, nSubPathIndex, pszSubPath);
    while ((pcSlash = strpbrk(pszSubPath, "\\/")) != NULL)
    {
        pszSubPath = pcSlash + 1;
        nSubPathIndex++;
        //printf("%s: Sub Path %d: %s.\r\n", __func__, nSubPathIndex, pszSubPath);
    }

    // Make Sure Slash Had Ever Been Found
    if (pszSubPath == (pszFullPath + 1))
    {
        printf("%s: No Directory In Path!\r\n", __func__);
        strcpy(pszFileName, pszFullPath);
        nRet = TP_ERR_FILE_NOT_FOUND;
        goto GET_FILE_NAME_EXIT;
    }

    // The Last SubPath is File Name
    printf("%s: Filename of Path \"%s\" is \"%s\".\r\n", __func__, pszFullPath, pszSubPath);
    strcpy(pszFileName, pszSubPath);

GET_FILE_NAME_EXIT:
    return nRet;
}

int CBaseLog::CreateDirPath(char *pszDirPath)
{
    int nRet = TP_SUCCESS,
        nDirNameLen = 0,
        nIndex = 0,
        nDir = 0;
    char szDirName[256] = {0},
                          szTempDirName[256] = {0};
    struct stat file_stat;

    // Make Sure Input Directory Path Valid
    if (pszDirPath == NULL)
    {
        //DBG("%s: Null Directory Path!", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto CREATE_DIR_PATH_EXIT;
    }

    // Make Sure Directory Not Exist
    if ((stat(pszDirPath, &file_stat) == 0) && (S_ISDIR(file_stat.st_mode)))
    {
        //DBG("%s: Directory \"%s\" Exists!", __func__, szDirPath);
        nRet = TP_ERR_INVALID_PARAM;
        goto CREATE_DIR_PATH_EXIT;
    }

    strcpy(szDirName, pszDirPath);
    nDirNameLen = strlen(szDirName);
#ifdef __linux__
    if (szDirName[nDirNameLen - 1] != '/')
    {
        strcat(szDirName, "/");
        nDirNameLen++;
    }
#else //_WIN32
    if (szDirName[nDirNameLen - 1] != '\\')
    {
        strcat(szDirName, "\\");
        nDirNameLen++;
    }
#endif //__linux__
    //DBG("%s: DirPath=\"%s\", DirName=\"%s\", DirNameLen=%d.", __func__, pszDirPath, szDirName, nDirNameLen);

    for (nIndex = 1 /* Skip root '/' */; nIndex < nDirNameLen; nIndex++)
    {
        if ((szDirName[nIndex] == '/') /*linux*/ || (szDirName[nIndex] == '\\') /* WIN32 */)
        {
            // Temp Directory Name
            memset(szTempDirName, 0, sizeof(szTempDirName));
            strncpy(szTempDirName, szDirName, nIndex);
            szTempDirName[nIndex] = '\0';

            // Check if directory has existed
            nDir = access(szTempDirName, 0);
            if (nDir != 0) // not exist
            {
                // Create new directory
                //DBG("Create Directory %s.", szTempDirName);
#ifdef __linux__
                nDir = mkdir(szTempDirName, S_IRWXU | S_IRWXG | S_IRWXO);
                if (nDir == -1) // create fail
#else // _WIN32
                nDir = CreateDirectoryA(szTempDirName, NULL);
                if (nDir == 0)
#endif //__linux__
                {
                    //ERR("Fail to create directory \"%s\".", szTempDirName);
                    printf("%s: Fail to create directory \"%s\".\r\n", __func__, szTempDirName);
                    nRet = TP_ERR_IO_ERROR;
                    break;
                }
            }
        }
    }

CREATE_DIR_PATH_EXIT:
    return nRet;
}

int CBaseLog::CleanFileContentWithPath(const char *pszFilePath)
{
    int nRet = TP_SUCCESS;
    FILE *fd = NULL;
    //printf("%s: Path=\"%s\".\r\n", __func__, pszFilePath);

    fd = fopen(pszFilePath, "w");
    if (fd == NULL)
    {
        printf("%s: Fail to open \"%s\"! (errno=%d)\r\n", __func__, pszFilePath, errno);
        nRet = TP_ERR_IO_ERROR;
    }
    else //if(fd != NULL)
        fclose(fd);

    return nRet;
}

int CBaseLog::CopyFileA(const char *pszSrcFilePath, char *pszDestFilePath)
{
    // BUFSIZE default is 8192 bytes
    // BUFSIZE of 1 means one chareter at time
    // good values should fit to blocksize, like 1024 or 4096
    // higher values reduce number of system calls
    // size_t BUFFER_SIZE = 4096;
    int nRet = TP_SUCCESS;
    char szBuf[BUFSIZ] = {0};
    size_t size = 0;
    FILE *fSource = NULL,
         *fDest = NULL;

    // Make Sure Path of Input & Output Files are Valid
    if (pszSrcFilePath == NULL)
    {
        printf("%s: NULL Source File Path!\r\n", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto COPY_FILE_EXIT;
    }
    if (pszDestFilePath == NULL)
    {
        printf("%s: NULL Destination File Path!\r\n", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto COPY_FILE_EXIT;
    }
    //printf("%s: from=\"%s\", to=\"%s\".\r\n", __func__, pszSrcFilePath, pszDestFilePath);

    // Open Source File
    //printf("%s: Open Source File \"%s\".\r\n", __func__, pszSrcFilePath);
    fSource = fopen(pszSrcFilePath, "rb");
    if (fSource == NULL)
    {
        printf("%s: Fail to open source file \"%s\"! (errno=%d)\r\n", __func__, pszSrcFilePath, errno);
        nRet = TP_ERR_IO_ERROR;
        goto COPY_FILE_EXIT;
    }

    // Open Destination File
    //printf("%s: Open Destination File \"%s\".\r\n", __func__, pszDestFilePath);
    fDest = fopen(pszDestFilePath, "ab+");
    if (fDest == NULL)
    {
        printf("%s: Fail to open destination file \"%s\"! (errno=%d)\r\n", __func__, pszDestFilePath, errno);
        nRet = TP_ERR_IO_ERROR;
        goto COPY_FILE_EXIT_1;
    }

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

    // clean and more secure
    //printf("%s: Write File.\r\n", __func__);
    // feof(FILE* stream) returns non-zero if the end of file indicator for stream is set
    while ((size = fread(szBuf, 1, BUFSIZ, fSource))) // double quote for "warning: suggest parentheses around assignment used as truth value"
        fwrite(szBuf, 1, size, fDest);

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__

    // Close Destination File
    if (fDest)
    {
        fclose(fDest);
        fDest = NULL;
    }

COPY_FILE_EXIT_1:
    // Close Source File
    if (fSource)
    {
        fclose(fSource);
        fSource = NULL;
    }

COPY_FILE_EXIT:
    return nRet;
}

int CBaseLog::SetLogDirPath(char *pszDirPath)
{
    int nRet = TP_SUCCESS;
    size_t nPathLen = 0;
    char szCurDebugLogFilePath[PATH_LEN_MAX] = {0},
         szCurTestResultLogFilePath[PATH_LEN_MAX] = {0};
#ifdef _WIN32 // Windows 32-bit Platform
    char szCurDirectoryPath[PATH_LEN_MAX] = {0},
         szCurAbsoluteDirPath[PATH_LEN_MAX] = {0};
#endif //_WIN32
    struct stat file_stat;
    //printf("%s: cur=\"%s\", new=\"%s\".\r\n", __func__, m_szLogDirPath, pszDirPath);

    // Make Sure Dir Path Valid
    if (pszDirPath == NULL)
    {
        printf("%s: NULL Log Dir Path!\r\n", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto SET_LOG_DIR_PATH_EXIT;
    }

    // Make Sure If Dir Path Exist
    if (strcmp(pszDirPath, "") == 0)
    {
        //printf("%s: Null Log Dir Path!\r\n", __func__);
        goto SET_LOG_DIR_PATH_EXIT;
    }

    // Make Sure If Dir Path Changed
    if (strcmp(m_szLogDirPath, pszDirPath) == 0)
    {
        //printf("%s: The Same Dir Path as Orginal One!\r\n", __func__);
        goto SET_LOG_DIR_PATH_EXIT;
    }

#ifdef _WIN32 // Windows 32-bit Platform
    // Transfer Relative Path to Absolute Path
    if (pszDirPath[0] == '.')
    {
        // Get Absolute Path of Current Directory
        GetCurrentDirectoryA(MAX_PATH, szCurDirectoryPath);
        //printf("%s: Current Direcotry: \"%s\".\r\n", __func__, szCurDirectoryPath);

        // Get Absolute Path to Log Dir Path
        sprintf(szCurAbsoluteDirPath, "%s", szCurDirectoryPath);
        strncpy(szCurAbsoluteDirPath, &pszDirPath[1], strlen(pszDirPath) - 1);
        //printf("%s: Absolute Log Dir Path: \"%s\".\r\n", __func__, szCurAbsoluteDirPath);

        // Make Sure If Dir Path Changed with Absolute Log Dir Path
        if (strcmp(m_szLogDirPath, szCurAbsoluteDirPath) == 0)
        {
            //printf("%s: Absolute Dir Path is the Same as Orginal One!\r\n", __func__);
            goto SET_LOG_DIR_PATH_EXIT;
        }
    }
#endif //_WIN32

    // Update Dir Path
    memset(m_szLogDirPath, 0, sizeof(m_szLogDirPath));
#ifdef _WIN32 // Windows 32-bit Platform
    if (pszDirPath[0] == '.')
        sprintf(m_szLogDirPath, "%s", szCurAbsoluteDirPath);
    else
#endif //_WIN32
        sprintf(m_szLogDirPath, "%s", pszDirPath);
    //printf("%s: LogDirPath=\"%s\".\r\n", __func__, m_szLogDirPath);

    // Remove the last '/' or '\'
    nPathLen = strlen(m_szLogDirPath);
    //printf("%s: LogDirPath[%zd]=\'%c\'.\r\n", __func__, nPathLen-1, m_szLogDirPath[nPathLen-1]);
    if ((m_szLogDirPath[nPathLen - 1] == '/') /* linux */ || (m_szLogDirPath[nPathLen - 1] == '\\' /* WIN32 */))
    {
        //printf("%s: Remove the last token '/' or '\' from path of log directory.", __func__);
        m_szLogDirPath[nPathLen - 1] = '\0';
        //printf("%s: LogDirPath = \"%s\".\r\n", __func__, m_szLogDirPath);
    }

    // Create Dir Path
    CreateDirPath(m_szLogDirPath);

    // Move Debug Log File If Exist
    if (strcmp(m_szDebugLogFileName, "") != 0)
    {
        // Backup Original Log File Path
        strcpy(szCurDebugLogFilePath, m_szDebugLogFilePath);

        // Update Debug Log File Path
        memset(m_szDebugLogFilePath, 0, sizeof(m_szDebugLogFilePath));
#ifdef __linux__
        sprintf(m_szDebugLogFilePath, "%s/%s", m_szLogDirPath, m_szDebugLogFileName);
#else //_WIN32
        sprintf(m_szDebugLogFilePath, "%s\\%s", m_szLogDirPath, m_szDebugLogFileName);
#endif //__linux__
        printf("%s: DebugLogFileName=\"%s\", DebugLogFilePath=\"%s\".\r\n", __func__, \
               m_szDebugLogFileName, m_szDebugLogFilePath);

        // Move Debug Log File If Exist
        if (stat(szCurDebugLogFilePath, &file_stat) == 0)
        {
            // Make Sure All Locks Unlocked
            assert(m_nFileIoLockCounter == 0);

            // Mutex locks the critical section
#ifdef __linux__
            sem_wait(&m_semFileIoMutex);
#else // _WIN32
            EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
            m_nFileIoLockCounter++;

            // Move Debug Log File
            //printf("%s: Move file \"%s\" to \"%s\".\r\n", __func__, szCurDebugLogFilePath, m_szDebugLogFilePath);
            rename(szCurDebugLogFilePath, m_szDebugLogFilePath);

            // Mutex unlocks the critical section
#ifdef __linux__
            sem_post(&m_semFileIoMutex);
#else // _WIN32
            LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
            m_nFileIoLockCounter--;
        }
    }

    // Move Test Result Log File If Exist
    if (strcmp(m_szTestResultLogFileName, "") != 0)
    {
        // Backup Original Log File Path
        strcpy(szCurTestResultLogFilePath, m_szTestResultLogFilePath);

        // Update Test Result Log File Path
        memset(m_szTestResultLogFilePath, 0, sizeof(m_szTestResultLogFilePath));
#ifdef __linux__
        sprintf(m_szTestResultLogFilePath, "%s/%s", m_szLogDirPath, m_szTestResultLogFileName);
#else //_WIN32
        sprintf(m_szTestResultLogFilePath, "%s\\%s", m_szLogDirPath, m_szTestResultLogFileName);
#endif //__linux
        //printf("%s: TestResultLogFilePath=\"%s\".\r\n", __func__, m_szTestResultLogFilePath);
        /*
        printf("%s: TestResultLogFileName=\"%s\", TestResultLogFilePath=\"%s\".\r\n", __func__, \
        				m_szTestResultLogFileName, m_szTestResultLogFilePath);
        */

        // Move Debug Log File If Exist
        if (stat(szCurTestResultLogFilePath, &file_stat) == 0)
        {
            // Make Sure All Locks Unlocked
            assert(m_nFileIoLockCounter == 0);

            // Mutex locks the critical section
#ifdef __linux__
            sem_wait(&m_semFileIoMutex);
#else // _WIN32
            EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
            m_nFileIoLockCounter++;

            // Move Test Result Log File
            //printf("%s: Move file \"%s\" to \"%s\".\r\n", __func__, szCurTestResultLogFilePath, m_szTestResultLogFilePath);
            rename(szCurTestResultLogFilePath, m_szTestResultLogFilePath);

            // Mutex unlocks the critical section
#ifdef __linux__
            sem_post(&m_semFileIoMutex);
#else // _WIN32
            LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
            m_nFileIoLockCounter--;
        }
    }

SET_LOG_DIR_PATH_EXIT:
    return nRet;
}

int CBaseLog::GetLogDirPath(char *pszDirPathBuffer)
{
    //char szLog[LOG_BUF_SIZE] = {0};

    // Make Sure Input Buffer Valid
    if (pszDirPathBuffer == NULL)
        return TP_ERR_INVALID_PARAM;

    //sprintf(szLog, "%s: LogDirPath=\"%s\".", __func__, m_szLogDirPath);
    //DebugLog(szLog);

    // Load Log Dir Path to Input Dir Path Buffer
    sprintf(pszDirPathBuffer, "%s", m_szLogDirPath);

    return TP_SUCCESS;
}

int CBaseLog::SetDebugLogFileName(char *pszFileName)
{
    int nRet = TP_SUCCESS;
    char szCurDebugLogFilePath[PATH_LEN_MAX] = {0};
    struct stat file_stat;
    //printf("%s: pszFileName=\"%s\".\r\n", __func__, pszFileName);

    // Make Sure File Name Valid
    if (pszFileName == NULL)
    {
        printf("%s: No Input File Name!\r\n", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto SET_DEBUG_LOG_FILE_NAME_EXIT;
    }

    // Make Sure If Debug Log File Name Exist
    if (strcmp(pszFileName, "") == 0)
    {
        //printf("%s: Null Debug Log File Name!\r\n", __func__);
        goto SET_DEBUG_LOG_FILE_NAME_EXIT;
    }

    // Make Sure Debug Log File Change
    if (strcmp(m_szDebugLogFileName, pszFileName) == 0)
    {
        printf("%s: Debug Log File Has Exist!\r\n", __func__);
        goto SET_DEBUG_LOG_FILE_NAME_EXIT;
    }

    // Backup Original Log File Path
    strcpy(szCurDebugLogFilePath, m_szDebugLogFilePath);

    // Update File Name
    memset(m_szDebugLogFileName, 0, sizeof(m_szDebugLogFileName));
    sprintf(m_szDebugLogFileName, "%s", pszFileName);
    //printf("%s: DebugLogFileName=\"%s\".\r\n", __func__, m_szDebugLogFileName);

    // Update File Path
    memset(m_szDebugLogFilePath, 0, sizeof(m_szDebugLogFilePath));
    if (strcmp(m_szLogDirPath, "") == 0) // Log Dir Path is Null
        sprintf(m_szDebugLogFilePath, "%s", m_szDebugLogFileName);
    else
#ifdef __linux__
        sprintf(m_szDebugLogFilePath, "%s/%s", m_szLogDirPath, m_szDebugLogFileName);
#else // _WIN32
        sprintf(m_szDebugLogFilePath, "%s\\%s", m_szLogDirPath, m_szDebugLogFileName);
#endif //__linux__
    //printf("%s: DebugLogFileName=\"%s\", DebugLogFilePath=\"%s\".\r\n", __func__, m_szDebugLogFileName, m_szDebugLogFilePath);

    // Move Debug Log File If Exist
    if (stat(szCurDebugLogFilePath, &file_stat) == 0)
    {
        // Make Sure All Locks Unlocked
        assert(m_nFileIoLockCounter == 0);

        // Mutex locks the critical section
#ifdef __linux__
        sem_wait(&m_semFileIoMutex);
#else // _WIN32
        EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
        m_nFileIoLockCounter++;

        // Move Debug Log File
        //printf("%s: Move file \"%s\" to \"%s\".\r\n", __func__, szCurDebugLogFilePath, m_szDebugLogFilePath);
        //CopyFileA(szCurDebugLogFilePath, m_szDebugLogFilePath);
        rename(szCurDebugLogFilePath, m_szDebugLogFilePath);

        // Remove Debug Log File
        //printf("%s: Remove file \"%s\" to \"%s\".\r\n", __func__, szCurDebugLogFilePath, m_szDebugLogFilePath);
        //remove(m_szDebugLogFilePath);

        // Mutex unlocks the critical section
#ifdef __linux__
        sem_post(&m_semFileIoMutex);
#else // _WIN32
        LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
        m_nFileIoLockCounter--;
    }

SET_DEBUG_LOG_FILE_NAME_EXIT:
    return nRet;
}

int CBaseLog::GetDebugLogFileName(char *pszFileNameBuffer)
{
    //char szLog[LOG_BUF_SIZE] = {0};

    // Make Sure Input Buffer Valid
    if (pszFileNameBuffer == NULL)
        return TP_ERR_INVALID_PARAM;

    //sprintf(szLog, "%s: DebugLogFileName=\"%s\".", __func__, m_szDebugLogFileName);
    //DebugLog(szLog);

    // Load Debug Log File Name to Input File Name Buffer
    sprintf(pszFileNameBuffer, "%s", m_szDebugLogFileName);

    return TP_SUCCESS;
}

int CBaseLog::GetDebugLogFilePath(char *pszFilePathBuffer)
{
    //char szLog[LOG_BUF_SIZE] = {0};

    // Make Sure Input Buffer Valid
    if (pszFilePathBuffer == NULL)
        return TP_ERR_INVALID_PARAM;

    //sprintf(szLog, "%s: DebugLogFilePath=\"%s\".", __func__, m_szDebugLogFilePath);
    //DebugLog(szLog);

    // Load Log File Path to Input Path Buffer
    sprintf(pszFilePathBuffer, "%s", m_szDebugLogFilePath);

    return TP_SUCCESS;
}

int CBaseLog::SetTestResultLogFileName(char *pszFileName)
{
    int nRet = TP_SUCCESS;
    char szCurTestResultLogFilePath[PATH_LEN_MAX] = {0};
    struct stat file_stat;

    // Make Sure File Name Valid
    if (pszFileName == NULL)
    {
        printf("%s: No Input File Name!\r\n", __func__);
        nRet = TP_ERR_INVALID_PARAM;
        goto SET_TEST_RESULT_LOG_FILE_NAME_EXIT;
    }

    // Make Sure If Test Result Log File Name Exist
    if (strcmp(pszFileName, "") == 0)
    {
        //printf("%s: Null Test Result Log File Name!\r\n", __func__);
        goto SET_TEST_RESULT_LOG_FILE_NAME_EXIT;
    }

    // Make Sure Test Result Log File Not Exist
    if (strcmp(m_szTestResultLogFileName, pszFileName) == 0)
    {
        printf("%s: Test Result Log File Has Exist!\r\n", __func__);
        goto SET_TEST_RESULT_LOG_FILE_NAME_EXIT;
    }

    // Backup Original Log File Path
    strcpy(szCurTestResultLogFilePath, m_szTestResultLogFilePath);

    // Update File Name
    memset(m_szTestResultLogFileName, 0, sizeof(m_szTestResultLogFileName));
    sprintf(m_szTestResultLogFileName, "%s", pszFileName);
    //printf("%s: TestResultLogFileName=\"%s\".\r\n", __func__, m_szTestResultLogFileName);

    // Update File Path
    memset(m_szTestResultLogFilePath, 0, sizeof(m_szTestResultLogFilePath));
    if (strcmp(m_szLogDirPath, "") == 0) // Log Dir Path is Null
        sprintf(m_szTestResultLogFilePath, "%s", m_szTestResultLogFileName);
    else
#ifdef __linux__
        sprintf(m_szTestResultLogFilePath, "%s/%s", m_szLogDirPath, m_szTestResultLogFileName);
#else //_WIN32
        sprintf(m_szTestResultLogFilePath, "%s\\%s", m_szLogDirPath, m_szTestResultLogFileName);
#endif //__linux__
    //printf("%s: TestResultLogFileName=\"%s\", TestResultLogFilePath=\"%s\".\r\n", __func__, m_szTestResultLogFileName, m_szTestResultLogFilePath);

    // Move Test Result Log File If Exist
    if (stat(szCurTestResultLogFilePath, &file_stat) == 0)
    {
        // Make Sure All Locks Unlocked
        assert(m_nFileIoLockCounter == 0);

        // Mutex locks the critical section
#ifdef __linux__
        sem_wait(&m_semFileIoMutex);
#else // _WIN32
        EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
        m_nFileIoLockCounter++;

        // Move Test Result Log File
        //printf("%s: Move file \"%s\" to \"%s\".\r\n", __func__, szCurTestResultLogFilePath, m_szTestResultLogFilePath);
        rename(szCurTestResultLogFilePath, m_szTestResultLogFilePath);

        // Remove Test Result Log File
        //printf("%s: Remove file \"%s\" to \"%s\".\r\n", __func__, szCurTestResultLogFilePath, m_szTestResultLogFilePath);
        //remove(m_szTestResultLogFilePath);

        // Mutex unlocks the critical section
#ifdef __linux__
        sem_post(&m_semFileIoMutex);
#else // _WIN32
        LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
        m_nFileIoLockCounter--;
    }

SET_TEST_RESULT_LOG_FILE_NAME_EXIT:
    return nRet;
}

int CBaseLog::GetTestResultLogFileName(char *pszFileNameBuffer)
{
    //char szLog[LOG_BUF_SIZE] = {0};

    // Make Sure Input Buffer Valid
    if (pszFileNameBuffer == NULL)
        return TP_ERR_INVALID_PARAM;

    //sprintf(szLog, "%s: TestResultLogFileName=\"%s\".", __func__, m_szTestResultLogFileName);
    //DebugLog(szLog);

    // Load Test Result Log File Name to Input File Name Buffer
    sprintf(pszFileNameBuffer, "%s", m_szTestResultLogFileName);

    return TP_SUCCESS;
}

int CBaseLog::GetTestResultLogFilePath(char *pszFilePathBuffer)
{
    //char szLog[LOG_BUF_SIZE] = {0};

    // Make Sure Input Buffer Valid
    if (pszFilePathBuffer == NULL)
        return TP_ERR_INVALID_PARAM;

    //sprintf(szLog, "%s: TestResultLogFilePath=\"%s\".", __func__, m_szTestResultLogFilePath);
    //DebugLog(szLog);

    // Load Test Result Log File Path to Input Path Buffer
    sprintf(pszFilePathBuffer, "%s", m_szTestResultLogFilePath);

    return TP_SUCCESS;
}
#endif //__ENABLE_LOG_FILE_DEBUG__

void CBaseLog::DebugLog(char *pszLog)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    FILE *fd = NULL;
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Make Sure Log String Valid
    if (pszLog == NULL)
        goto DEBUG_LOG_EXIT;

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

#ifdef __ENABLE_SYSLOG_DEBUG__
    syslog(LOG_DEBUG, "%s\n", pszLog);
#endif //__ENABLE_SYSLOG_DEBUG__

#ifdef __ENABLE_LOG_FILE_DEBUG__
    fd = fopen(m_szDebugLogFilePath, "a+");
    if (fd == NULL)
    {
        printf("%s: Fail to open \"%s\"! (errno=%d)\r\n", __func__, m_szDebugLogFilePath, errno);
        goto DEBUG_LOG_EXIT_1;
    }

    // Clear Buffer
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    // Get Current Time
    time(&m_tRawTime);
    strftime(m_szDateBuffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_tRawTime));
    gettimeofday(&m_tvCurTime, NULL);
    sprintf(m_szDateTimeBuffer, "%s:%03d:%03d", m_szDateBuffer, (int)m_tvCurTime.tv_usec / 1000, (int)m_tvCurTime.tv_usec % 1000);

    //Write data to file
    fprintf(fd, "%s [DEBUG] %s\n", m_szDateTimeBuffer, pszLog);

    fclose(fd);

DEBUG_LOG_EXIT_1:
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter--;

DEBUG_LOG_EXIT:
    return;
};

void CBaseLog::DebugLogFormat(const char *pszFormat, ...)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    FILE *fd = NULL;
#endif //__ENABLE_LOG_FILE_DEBUG__
    char szLogBuffer[LOG_BUF_SIZE] = {0};
    va_list pArgs;

    // Make Sure Log String Valid
    if (pszFormat == NULL)
        goto DEBUG_LOG_FORMAT_EXIT;

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

#ifdef __ENABLE_SYSLOG_DEBUG__
    // Load String to LogBuffer with Variable Argument List
    va_start(pArgs, pszFormat);
    vsnprintf(szLogBuffer, sizeof(szLogBuffer), pszFormat, pArgs);
    va_end(pArgs);

    syslog(LOG_DEBUG, "%s\n", szLogBuffer);
#endif //__ENABLE_SYSLOG_DEBUG__

#ifdef __ENABLE_LOG_FILE_DEBUG__
    // Make Sure Log File Exist
    fd = fopen(m_szDebugLogFilePath, "a+");
    if (fd == NULL)
    {
        printf("%s: Fail to open \"%s\"! (errno=%d)\r\n", __func__, m_szDebugLogFilePath, errno);
        goto DEBUG_LOG_FORMAT_EXIT_1;
    }

    // Clear Buffer
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    // Get Current Time
    time(&m_tRawTime);
    strftime(m_szDateBuffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_tRawTime));
    gettimeofday(&m_tvCurTime, NULL);
    sprintf(m_szDateTimeBuffer, "%s:%03d:%03d", m_szDateBuffer, (int)m_tvCurTime.tv_usec / 1000, (int)m_tvCurTime.tv_usec % 1000);

    // Load String to LogBuffer with Variable Argument List
    va_start(pArgs, pszFormat);
    vsnprintf(szLogBuffer, sizeof(szLogBuffer), pszFormat, pArgs);
    va_end(pArgs);

    //Write data to file
    fprintf(fd, "%s [DEBUG] %s\n", m_szDateTimeBuffer, szLogBuffer);

    fclose(fd);

DEBUG_LOG_FORMAT_EXIT_1:
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter--;

DEBUG_LOG_FORMAT_EXIT:
    return;
}

void CBaseLog::ErrorLog(char *pszLog)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    FILE *fd = NULL;
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Make Sure Log String Valid
    if (pszLog == NULL)
        goto ERROR_LOG_EXIT;

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

#ifdef __ENABLE_SYSLOG_DEBUG__
    syslog(LOG_ERR, "%s\n", pszLog);
#endif //__ENABLE_SYSLOG_DEBUG__

#ifdef __ENABLE_LOG_FILE_DEBUG__
    fd = fopen(m_szDebugLogFilePath, "a+");
    if (fd == NULL)
    {
        printf("%s: Fail to open \"%s\"! (errno=%d)\r\n", __func__, m_szDebugLogFilePath, errno);
        goto ERROR_LOG_EXIT_1;
    }

    // Clear Buffer
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    // Get Current Time
    time(&m_tRawTime);
    strftime(m_szDateBuffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_tRawTime));
    gettimeofday(&m_tvCurTime, NULL);
    sprintf(m_szDateTimeBuffer, "%s:%03d:%03d", m_szDateBuffer, (int)m_tvCurTime.tv_usec / 1000, (int)m_tvCurTime.tv_usec % 1000);

    //Write data to file
    fprintf(fd, "%s [ERROR] %s\n", m_szDateTimeBuffer, pszLog);

    fclose(fd);

ERROR_LOG_EXIT_1:
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter--;

ERROR_LOG_EXIT:
    return;
};

void CBaseLog::ErrorLogFormat(const char *pszFormat, ...)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    FILE *fd = NULL;
#endif //__ENABLE_SYSLOG_DEBUG__
    char szLogBuffer[LOG_BUF_SIZE] = {0};
    va_list pArgs;

    // Make Sure Log String Valid
    if (pszFormat == NULL)
        goto ERROR_LOG_FORMAT_EXIT;

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

#ifdef __ENABLE_SYSLOG_DEBUG__
    // Load String to LogBuffer with Variable Argument List
    va_start(pArgs, pszFormat);
    vsnprintf(szLogBuffer, sizeof(szLogBuffer), pszFormat, pArgs);
    va_end(pArgs);

    syslog(LOG_ERR, "%s\n", szLogBuffer);
#endif //__ENABLE_SYSLOG_DEBUG__

#ifdef __ENABLE_LOG_FILE_DEBUG__
    fd = fopen(m_szDebugLogFilePath, "a+");
    if (fd == NULL)
    {
        printf("%s: Fail to open \"%s\"! (errno=%d)\r\n", __func__, m_szDebugLogFilePath, errno);
        goto ERROR_LOG_FORMAT_EXIT_1;
    }

    // Clear Buffer
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    // Get Current Time
    time(&m_tRawTime);
    strftime(m_szDateBuffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_tRawTime));
    gettimeofday(&m_tvCurTime, NULL);
    sprintf(m_szDateTimeBuffer, "%s:%03d:%03d", m_szDateBuffer, (int)m_tvCurTime.tv_usec / 1000, (int)m_tvCurTime.tv_usec % 1000);

    // Load String to LogBuffer with Variable Argument List
    va_start(pArgs, pszFormat);
    vsnprintf(szLogBuffer, sizeof(szLogBuffer), pszFormat, pArgs);
    va_end(pArgs);

    //Write data to file
    fprintf(fd, "%s [ERROR] %s\n", m_szDateTimeBuffer, szLogBuffer);

    fclose(fd);

ERROR_LOG_FORMAT_EXIT_1:
#endif //__ENABLE_SYSLOG_DEBUG__

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter--;

ERROR_LOG_FORMAT_EXIT:
    return;
}

void CBaseLog::DebugPrintBuffer(unsigned char *pbyBuf, int nLen)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    FILE *fd = NULL;
#endif //__ENABLE_LOG_FILE_DEBUG__
    int nIndex = 0;
    unsigned char *pbyData = NULL;
    char szBuffer[8] = { 0 };

    if (pbyBuf == NULL)
    {
        printf("%s: Input Buffer is NULL!\r\n", __func__);
        goto DEBUG_PRINT_BUFFER_EXIT;
    }

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

#ifdef __ENABLE_SYSLOG_DEBUG__
    // Set data to buffer
    memset(m_szDebugBuf, 0, sizeof(m_szDebugBuf));
    for (nIndex = 0, pbyData = pbyBuf; nIndex < nLen; nIndex++, pbyData++)
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, " %02x", *pbyData);
        strcat(m_szDebugBuf, szBuffer);
    }

    syslog(LOG_DEBUG, "buffer[%d]=%s.\n", nIndex, m_szDebugBuf);
#endif //__ENABLE_SYSLOG_DEBUG__

#ifdef __ENABLE_LOG_FILE_DEBUG__
    fd = fopen(m_szDebugLogFilePath, "a+");
    if (fd == NULL)
    {
        printf("%s: Fail to open file \"%s\"! (errno=%d)\r\n", __func__, m_szDebugLogFilePath, errno);
        goto DEBUG_PRINT_BUFFER_EXIT_1;
    }

    // Set data to buffer
    memset(m_szDebugBuf, 0, sizeof(m_szDebugBuf));
    for (nIndex = 0, pbyData = pbyBuf; nIndex < nLen; nIndex++, pbyData++)
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, " %02x", *pbyData);
        strcat(m_szDebugBuf, szBuffer);
    }

    // Clear Buffer
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    // Get Current Time
    time(&m_tRawTime);
    strftime(m_szDateBuffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_tRawTime));
    gettimeofday(&m_tvCurTime, NULL);
    sprintf(m_szDateTimeBuffer, "%s:%03d:%03d", m_szDateBuffer, (int)m_tvCurTime.tv_usec / 1000, (int)m_tvCurTime.tv_usec % 1000);

    // Write buffer to file
    fprintf(fd, "%s [DEBUG] buffer[%d]=%s.\n", m_szDateTimeBuffer, nIndex, m_szDebugBuf);

    fclose(fd);

DEBUG_PRINT_BUFFER_EXIT_1:
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter--;

DEBUG_PRINT_BUFFER_EXIT:
    return;
}

void CBaseLog::DebugPrintBuffer(const char *pszBufName, unsigned char *pbyBuf, int nLen)
{
#ifdef __ENABLE_LOG_FILE_DEBUG__
    FILE *fd = NULL;
#endif //__ENABLE_LOG_FILE_DEBUG__
    int nIndex = 0;
    unsigned char *pbyData = NULL;
    char szBuffer[8] = { 0 };

    if ((!pszBufName) || (!pbyBuf))
    {
        printf("%s: buf_name = %p, buf = %p, return!\r\n", __func__, pszBufName, pbyBuf);
        goto DEBUG_PRINT_BUFFER_2_EXIT;
    }

    // Make Sure All Locks Unlocked
    assert(m_nFileIoLockCounter == 0);

    // Mutex locks the critical section
#ifdef __linux__
    sem_wait(&m_semFileIoMutex);
#else // _WIN32
    EnterCriticalSection(&m_csFileIoMutex);
#endif //__linux
    m_nFileIoLockCounter++;

#ifdef __ENABLE_SYSLOG_DEBUG__
    // Set data to buffer
    memset(m_szDebugBuf, 0, sizeof(m_szDebugBuf));
    for (nIndex = 0, pbyData = pbyBuf; nIndex < nLen; nIndex++, pbyData++)
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, " %02x", *pbyData);
        strcat(m_szDebugBuf, szBuffer);
    }

    syslog(LOG_DEBUG, "%s[%d]=%s.\n", pszBufName, nIndex, m_szDebugBuf);
#endif //__ENABLE_SYSLOG_DEBUG__

#ifdef __ENABLE_LOG_FILE_DEBUG__
    fd = fopen(m_szDebugLogFilePath, "a+");
    if (fd == NULL)
    {
        printf("%s: Fail to open file \"%s\"! (errno=%d)\r\n", __func__, m_szDebugLogFilePath, errno);
        goto DEBUG_PRINT_BUFFER_2_EXIT_1;
    }

    // Set data to buffer
    memset(m_szDebugBuf, 0, sizeof(m_szDebugBuf));
    for (nIndex = 0, pbyData = pbyBuf; nIndex < nLen; nIndex++, pbyData++)
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, " %02x", *pbyData);
        strcat(m_szDebugBuf, szBuffer);
    }

    // Clear Buffer
    memset(m_szDateBuffer, 0, sizeof(m_szDateBuffer));
    memset(m_szDateTimeBuffer, 0, sizeof(m_szDateTimeBuffer));

    // Get Current Time
    time(&m_tRawTime);
    gettimeofday(&m_tvCurTime, NULL);
    strftime(m_szDateBuffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_tRawTime));
    sprintf(m_szDateTimeBuffer, "%s:%03d:%03d", m_szDateBuffer, (int)m_tvCurTime.tv_usec / 1000, (int)m_tvCurTime.tv_usec % 1000);

    // Write buffer to file
    fprintf(fd, "%s [DEBUG] %s[%d]=%s.\n", m_szDateTimeBuffer, pszBufName, nIndex, m_szDebugBuf);

    fclose(fd);

DEBUG_PRINT_BUFFER_2_EXIT_1:
#endif //__ENABLE_LOG_FILE_DEBUG__

    // Mutex unlocks the critical section
#ifdef __linux__
    sem_post(&m_semFileIoMutex);
#else // _WIN32
    LeaveCriticalSection(&m_csFileIoMutex);
#endif // __linux__
    m_nFileIoLockCounter--;

DEBUG_PRINT_BUFFER_2_EXIT:
    return;
}

