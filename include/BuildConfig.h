#ifndef _BUILD_CONFIG_H_932565_98578723_9836_682C
#define _BUILD_CONFIG_H_932565_98578723_9836_682C

#ifdef __linux__
#include <sys/time.h>	// gettimeofday
#include <semaphore.h>	// semaphore
#include <unistd.h>		// usleep

#define SleepMS( ms )	usleep( (ms) * 1000)

// Max Length of Full Path Name
#ifndef MAX_PATH
#define MAX_PATH        260
#endif //MAX_PATH

// Type Re-define
typedef unsigned char       byte;
typedef unsigned short      word;

// WIN32 Definition
typedef void*	HWND;
#endif // __linux__

#ifdef WIN32

#pragma warning(disable: 4100)

#include <Windows.h>
#include <fstream>
#include <math.h>
#include <assert.h>
#include <mmsystem.h>

typedef unsigned char* PBYTE;
typedef unsigned char byte;

#include "windows_debug_utility.h"

//Let the HIDWinGet and SocketGet interface exist.
#include "HIDWinGet.h"
#if defined(INTERFACE_SOCKET)
#include "SocketGet.h"
#endif

#define UNREFERENCE(x)		// Do nothing. just for notification.

//
// million second
//
#define SleepMS(ms)	Sleep(ms)

#endif	//#ifdef WIN32
#endif // _BUILD_CONFIG_H_932565_98578723_9836_682
