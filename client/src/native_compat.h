#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int UINT;
typedef int BOOL;
typedef char CHAR;
typedef float FLOAT;
typedef char *PCHAR;
typedef void *PVOID;
typedef void *HANDLE;
typedef void *HWND;
typedef void *HFONT;
typedef void *HINSTANCE;
typedef char *LPSTR;
typedef char *LPTSTR;
typedef char TCHAR;
typedef long LRESULT;
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef int SOCKET;

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif
#define APIENTRY
#define WINAPI
#define CALLBACK
#define TRUE 1
#define FALSE 0
#define MB_ICONERROR 0

struct SYSTEMTIME
{
	WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};

inline DWORD GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return static_cast<DWORD>((tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL) & 0xFFFFFFFFU);
}

inline void Sleep(DWORD milliseconds) { usleep(milliseconds * 1000U); }

inline void GetLocalTime(SYSTEMTIME *result)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm local;
	localtime_r(&tv.tv_sec, &local);
	result->wYear = local.tm_year + 1900;
	result->wMonth = local.tm_mon + 1;
	result->wDay = local.tm_mday;
	result->wDayOfWeek = local.tm_wday;
	result->wHour = local.tm_hour;
	result->wMinute = local.tm_min;
	result->wSecond = local.tm_sec;
	result->wMilliseconds = tv.tv_usec / 1000;
}

inline void ExitProcess(int code) { _exit(code); }
inline int MessageBox(void *, const char *message, const char *, int) { std::fprintf(stderr, "%s\n", message); return 0; }
inline char *GetCommandLineA() { static char empty[] = ""; return empty; }
inline int SetWindowText(HWND, const char *) { return 0; }
inline int SetConsoleTitle(const char *) { return 0; }

inline int sprintf_s(char *buffer, size_t size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int result = vsnprintf(buffer, size, format, args);
	va_end(args);
	return result;
}

inline int vsprintf_s(char *buffer, size_t size, const char *format, va_list args)
{
	return vsnprintf(buffer, size, format, args);
}

inline int strcpy_s(char *destination, size_t size, const char *source)
{
	if(!destination || !source || size == 0)
		return 1;
	std::snprintf(destination, size, "%s", source);
	return 0;
}

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _copysign copysign
