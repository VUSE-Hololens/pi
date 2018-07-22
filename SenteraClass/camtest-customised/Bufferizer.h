#ifndef BUFFERIZER_H_
#define BUFFERIZER_H_
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
#include "Packetizer.h"
#include "crc.h"
#include "suas_types.h"
#include "packets.h"

class Bufferizer
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
    static int session(fw_imager_session_t& session, uint8_t *buf);
    static int video(fw_video_session_t& video_session, uint8_t *buf);
    static int videotime(fw_video_session_t& video_session, uint8_t *buf);
    static int videoadvanced(fw_video_session_advanced_t& video_session_adv, uint8_t *buf);
    static int videoadjust(fw_video_adjust_t& video_adjust, uint8_t *buf);
    static int exposureadjust(fw_exposure_adjust_t& exposure_adjust, uint8_t *buf);
    static int focus(fw_focus_session_t& focus, uint8_t *buf);
    static int sf(fw_still_focus_session_t& still_focus, uint8_t *buf);
    static int trigger(fw_imager_trigger_t& trigger, uint8_t *buf);
    static int zoom(fw_imager_zoom_t& zoom, uint8_t *buf);
    static int preview_stream_setup(fw_imager_preview_stream_setup_t& preview, uint8_t *buf);
    static int elevation_metadata(fw_elevation_metadata_t& elevation, uint8_t *buf);
    static int aircraft_metadata(fw_aircraft_metadata_t& aircraft, uint8_t *buf);
    static int system_time(fw_system_time_t& time, uint8_t *buf);
    static int video_adjust_relative(fw_video_adjust_relative_t& adjust, uint8_t *buf);

// *****************************************************************************
// ************************** Protected Static Functions ***********************
// *****************************************************************************
protected:    

// *****************************************************************************
// ************************** Private Static Functions *************************
// *****************************************************************************
private:
    static uint8_t CRC8_7(uint8_t cur_crc, uint8_t *msg, uint32_t msg_size);

    Bufferizer() 
    {
        // Emptry construcutor for static class
    };
};
#endif // BUFFERIZER_H_
