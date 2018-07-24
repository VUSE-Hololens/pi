#ifndef PACKETIZER_H_
#define PACKETIZER_H_

// **************************** System Includes ********************************
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <cstdlib>

// UNIX ONLY Libraries
#include <sys/time.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// UNIX socket library
#include <sys/socket.h>

// **************************** User Includes **********************************
#include "packets.h"

class DataPacketizer
{

public:
    static fw_imager_session_t session(uint8_t command, const char name[]);
    static fw_imager_trigger_t trigger(uint8_t trigger_mask, uint8_t option, uint32_t interval);
    static fw_imager_zoom_t zoom(uint8_t trigger_mask);
    static fw_imager_preview_stream_setup_t preview_stream_setup(uint8_t trigger_mask);
    static fw_elevation_metadata_t elevation_metadata();
    static fw_aircraft_metadata_t location_metadata();
    static fw_video_session_t video();
    static fw_video_session_t videotime(struct timeval &currTv);
    static fw_video_session_advanced_t videoadvanced(struct timeval &currTv);
    static fw_exposure_adjust_t exposureadjust();
    static fw_focus_session_t focus();
    static fw_still_focus_session_t sf();
    static fw_imager_trigger_t spoof(uint8_t trigger_mask);
	//TODO: static fw_system_time_t system_time();
	static fw_video_adjust_t video_adjust();
    static fw_video_adjust_relative_t video_adjust_relative(uint8_t trigger_mask);


// *****************************************************************************
// ************************** Private Static Functions *************************
// *****************************************************************************
private:
    DataPacketizer() 
    {
        // Private constructor for static class
    }
};
#endif // PACKETIZER_H_