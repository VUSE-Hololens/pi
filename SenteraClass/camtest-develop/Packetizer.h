#ifndef PACKETIZER_H_
#define PACKETIZER_H_
// *****************************************************************************
// **************************** System Includes ********************************
// *****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// Define IS_WINDOWS if compiled on a Windows machine
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__))
#define IS_WINDOWS (1==1)
#endif

// Include additional libraries & declare variables based on operating system
#if IS_WINDOWS
// For WinSock 2 & Windows API
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

// For polling user input
#include <conio.h>

// Tell linker Ws2_32.lib file is needed
#pragma comment(lib, "Ws2_32.lib")
#else
// UNIX ONLY Libraries
#include <sys/time.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// UNIX socket library
#include <sys/socket.h>
#endif
// *****************************************************************************
// **************************** User Includes **********************************
// *****************************************************************************
#include "crc.h"
#include "suas_types.h"
#include "packets.h"
#if IS_WINDOWS
#include "UnixForWindows.h"
#endif



class Packetizer
{
// =============================================================================
// *****************************************************************************
// ************************** Public Member Variables **************************
// *****************************************************************************
public:

// *****************************************************************************
// ************************** Protected Member Variables ***********************
// *****************************************************************************
protected:

// *****************************************************************************
// ************************** Private Member Variables *************************
// *****************************************************************************
private:

// =============================================================================
// *****************************************************************************
// ************************** Public Static Variables **************************
// *****************************************************************************
public:

// *****************************************************************************
// ************************** Protected Static Variables ***********************
// *****************************************************************************
protected:

// *****************************************************************************
// ************************** Private Static Variables *************************
// *****************************************************************************
private:

// =============================================================================
// *****************************************************************************
// ************************** Public Member Functions **************************
// *****************************************************************************
public:

// *****************************************************************************
// ************************** Protected Member Functions ***********************
// *****************************************************************************
protected:

// *****************************************************************************
// ************************** Private Member Functions *************************
// *****************************************************************************
private:

// =============================================================================
// *****************************************************************************
// ************************** Public Static Functions **************************
// *****************************************************************************
public:
    static fw_imager_session_t session();
    static fw_imager_trigger_t trigger(uint8 trigger_mask);
    static fw_imager_zoom_t zoom(uint8 trigger_mask);
    static fw_imager_preview_stream_setup_t preview_stream_setup(uint8 trigger_mask);
    static fw_elevation_metadata_t elevation_metadata();
    static fw_aircraft_metadata_t location_metadata();
    static fw_video_session_t video();
    static fw_video_session_t videotime(struct timeval &currTv);
    static fw_video_session_advanced_t videoadvanced(struct timeval &currTv);
    static fw_exposure_adjust_t exposureadjust();
    static fw_focus_session_t focus();
    static fw_still_focus_session_t sf();
    static fw_imager_trigger_t spoof(uint8 trigger_mask);
	static fw_system_time_t system_time();
    static fw_video_adjust_relative_t video_adjust_relative(uint8 trigger_mask);


// *****************************************************************************
// ************************** Protected Static Functions ***********************
// *****************************************************************************
protected:

// *****************************************************************************
// ************************** Private Static Functions *************************
// *****************************************************************************
private:
    Packetizer() 
    {
        // Private constructor for static class
    }
};
#endif // PACKETIZER_H_