// Includes Windows implementations of several popular UNIX functions used in
// configuring pipes and timestamps
#ifndef UNIXFORWINDOWS_H
#define UNIXFORWINDOWS_H
// *****************************************************************************
// ************************** System Includes **********************************
// *****************************************************************************
#include <Windows.h>

// *****************************************************************************
// ************************** Global Functions *********************************
// *****************************************************************************
// For Windows implementation of gettimeofday()
static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);

// Windows-friendly implementation of gettimeofday; initializes timeval object to the time of day
// http://stackoverflow.com/questions/1676036/what-should-i-use-to-replace-gettimeofday-on-windows
int gettimeofday(struct timeval * tp, struct timezone * tzp);

// Windows-friendly implentation of usleep; sleeps thread for specified time (useconds)
// https://www.c-plusplus.net/forum/109539-full
void usleep(unsigned int usec);

#endif