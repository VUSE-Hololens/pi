// *****************************************************************************
// ************************** System Includes **********************************
// *****************************************************************************
#include <Windows.h>

// *****************************************************************************
// **************************** User Includes **********************************
// *****************************************************************************
#include "UnixForWindows.h"

// Windows-friendly implementation of gettimeofday; initializes timeval object to the time of day
// http://stackoverflow.com/questions/1676036/what-should-i-use-to-replace-gettimeofday-on-windows
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    return 0;
}

// Windows-friendly implentation of usleep; sleeps thread for specified time (useconds)
// https://www.c-plusplus.net/forum/109539-full
void usleep(unsigned int usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * (__int64)usec);

    //Timer Funktionen ab WINNT verfügbar 
    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}