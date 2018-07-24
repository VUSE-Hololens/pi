// *****************************************************************************
// **************************** System Includes ********************************
// *****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <cstdlib>

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
#if IS_WINDOWS
#include "UnixForWindows.h"
#endif

// *****************************************************************************
// ********************************* DEFINES ***********************************
// *****************************************************************************
#if IS_WINDOWS
// Define type for preview command
typedef uint32_t in_addr_t;
#endif

// *****************************************************************************
// ************************** Public Static Functions **************************
// *****************************************************************************
// @brief Builds session packet
// @param none
// @return Imager session packet
fw_imager_session_t Packetizer::session()
{
    // Initialize packet and clear the needed memory
    fw_imager_session_t imager_session;
    memset(&imager_session, 0, sizeof(imager_session));

    // Open, close, or kill session according to user input
    uint32 tmp_u32;
    printf("open (0), close (1), or kill (2): ");
    scanf("%u", &tmp_u32);
    imager_session.sessionCmd = tmp_u32 & 0xFF;

    if (imager_session.sessionCmd == 0)
    {
        // Use session name provided by user.
        printf("Session Name: ");
        scanf("%s", imager_session.sessionName);

        // Add UTC time data to the session open packet.
        printf("Append (0-no / 1-yes): ");
        scanf("%u", &tmp_u32);
        imager_session.resumeSession = tmp_u32 & 0xF;
        time_t raw_time;
        struct tm *utc_time;
        time(&raw_time);
        utc_time = gmtime(&raw_time);

        // Set packet elements for new session
        imager_session.utcYear = utc_time->tm_year;
        imager_session.utcMonth = utc_time->tm_mon;
        imager_session.utcDay = utc_time->tm_mday;
        imager_session.utcHour = utc_time->tm_hour;
        imager_session.utcMinute = utc_time->tm_min;
        imager_session.utcMillisecond = utc_time->tm_sec * 1000;
        imager_session.buildVersion = 0xFFFF;
        imager_session.aircraftType = 0xFF;

        // Use fixed session ID for testing.
        imager_session.sessionID = 1;
    }

    // Return the constructed packet
    return imager_session;
}

// @brief Builds trigger command packet
// @param trigger_mask Specifies which sensors are involved
// @return Imager trigger packet
fw_imager_trigger_t Packetizer::trigger(uint8 trigger_mask)
{
    // Initialize packet and clear the needed memory
    fw_imager_trigger_t imager_trigger;
    memset(&imager_trigger, 0, sizeof(imager_trigger));
    
    // Initialize trigger mode from user input
    uint32 tmp_u32;
    printf("\n");
    printf("Current Trigger Mask: 0x%02X\n", trigger_mask);
    printf("Disable (0), Single (1), or Continuous (2): ");
    scanf("%x", &tmp_u32);
    imager_trigger.trigMode = tmp_u32 & 0xFF;

    // Prompt user for trigger interval if applicable
    if (imager_trigger.trigMode == 2)
    {
        printf("Interval (ms): ");
        scanf("%u", &tmp_u32);
        imager_trigger.trigPeriod = tmp_u32 & 0xFFFF;
    }

    // Initialize imager select element from trigger mask
    imager_trigger.imgSelect = trigger_mask;

    // Initialize trigger ID element
    imager_trigger.trigID = 0x77;

    /// Return the constructed packet
    return imager_trigger;
}

// @brief Builds zoom packet
// @param trigger_mask Specifies which sensors are involved
// @return Zoom packet
fw_imager_zoom_t Packetizer::zoom(uint8 trigger_mask)
{
    // Initialize packet and clear the needed memory
    fw_imager_zoom_t imager_zoom;
    memset(&imager_zoom, 0, sizeof(imager_zoom));
    
    // Initialize zoom mode from user input
    int32 tmp_s32;
    uint32 tmp_u32;
    printf("Mode - Rate (1), Steps (2): ");
    scanf("%u", &tmp_u32);
    imager_zoom.zoomMode = tmp_u32 & 0xFF;

    // Initialize value for rate or steps from user input as applicable
    if (imager_zoom.zoomMode == 1)
    {
        printf("Rate: ");
        scanf("%d", &tmp_s32);
        imager_zoom.zoomRate = tmp_s32 & 0xFF;
    }
    else if (imager_zoom.zoomMode == 2)
    {
        printf("Steps: ");
        scanf("%d", &tmp_s32);
        imager_zoom.zoomSteps = tmp_s32 & 0xFF;
    }

    // Initialize imager select element from trigger mask
    imager_zoom.imgSelect = trigger_mask;

    // Return the constructed packet
    return imager_zoom;
}

// @brief Builds preview stream setup packet
// @param trigger_mask Specifies which sensors are involved
// @return Preview stream setup packet
fw_imager_preview_stream_setup_t Packetizer::preview_stream_setup(uint8 trigger_mask)
{
    // Initialize packet and clear the needed memeory
    fw_imager_preview_stream_setup_t imager_preview;
    memset(&imager_preview, 0, sizeof(imager_preview));
    
    // Initialize stream status from user input
    uint32 tmp_u32;
    printf("Enable(1), Disable(0): ");
    scanf("%d", &tmp_u32);
    imager_preview.videoStatus = tmp_u32 & 0xFF;

    // Prompt user for stream information
    char ip_address_dest[20];
    in_addr_t dest_in_addr;
    if (imager_preview.videoStatus == 1)
    {
        // Initialize stream's destination IP from user input
        printf("IP Address: ");
        scanf("%s", ip_address_dest);

        // Initialize destination IP element from user input
        dest_in_addr = inet_addr(ip_address_dest);
        imager_preview.videoDstIP = ntohl((uint32_t)dest_in_addr);
        printf("Dest IP: %x\n", imager_preview.videoDstIP);

        // Initialize destination port element from user input
        printf("Port: ");
        scanf("%d", &tmp_u32);
        imager_preview.videoDstPort = tmp_u32 & 0xFFFF;

        // Read in preview options for camera 0
        printf("Cam 0 Pos [NoChange(0), Disable(1), Full(2), PIP(3), Top(4), Bottom(5), Overlay(6)]: ");
        scanf("%d", &tmp_u32);
        imager_preview.cameraConfig += ((tmp_u32 & 0xFF) << 0);
        printf("Cam 0 Opt [NoChange(0), Normal(1), Live NDVI(2)]: ");
        scanf("%d", &tmp_u32);
        imager_preview.cameraConfig += ((tmp_u32 & 0xFF) << 8);

        // Read in preview options for camera 1
        printf("Cam 1 Pos [NoChange(0), Disable(1), Full(2), PIP(3), Top(4), Bottom(5), Overlay(6)]: ");
        scanf("%d", &tmp_u32);
        imager_preview.cameraConfig += ((tmp_u32 & 0xFF) << 16);
        printf("Cam 1 Opt [NoChange(0), Normal(1), Live NDVI(2)]: ");
        scanf("%d", &tmp_u32);
        imager_preview.cameraConfig += ((tmp_u32 & 0xFF) << 24);

        printf("Overlay NoChange(0), Disable(1), Enable(2): ");
        scanf("%d", &tmp_u32);
        imager_preview.overlayConfig = (tmp_u32 & 0xFF);
    }

    // Initialize imager select element from trigger mask
    imager_preview.imgSelect = trigger_mask;

    // Return the constructed packet
    return imager_preview;
}

// @brief Builds elevation metadata packet
// @param none
// @return Elevation metadata packet
fw_elevation_metadata_t Packetizer::elevation_metadata()
{
    // Initialize packet and clear the needed memeory
    fw_elevation_metadata_t elevation;
    memset(&elevation, 0, sizeof(elevation));
    
    // Initialize elevation mode from user input
    uint32 tmp_u32;
    printf("Mode - Camera AGL (0), Terrain MSL (1), Camera MSL (2): ");
    scanf("%u", &tmp_u32);
    elevation.mode = tmp_u32 & 0xFF;

    // Initialize data valid element accordingly
    if (elevation.mode == 0)
    {
        elevation.dataValid = 1;
    }
    else if (elevation.mode == 1)
    {
        elevation.dataValid = 2;
    }
    else if (elevation.mode == 2)
    {
        elevation.dataValid = 6;
    }
    else
    {
        printf("Valid Bit Flag - Camera MSL (4), Terrain MSL (2), Camera AGL (1): ");
        scanf("%u", &tmp_u32);
        elevation.dataValid = tmp_u32 & 0xFF;
    }

    // Initialize camera AGL from user input if applicable
    if (elevation.dataValid & 0x01)
    {
        printf("Camera AGL (m): ");
        scanf("%f", &elevation.cameraAGL);
    }
    else
    {
        elevation.cameraAGL = 0;
    }

    // Initialize terrain MSL from user input if applicable
    if (elevation.dataValid & 0x02)
    {
        printf("Terrain MSL (m): ");
        scanf("%f", &elevation.terrainMSL);
    }
    else
    {
        elevation.terrainMSL = 0;
    }

    // Initialize camera MSL from user input if applicable
    if (elevation.dataValid & 0x04)
    {
        printf("Camera MSL (m): ");
        scanf("%f", &elevation.cameraMSL);
    }
    else
    {
        elevation.cameraMSL = 0;
    }

    // Return the constructed packet
    return elevation;
}

// @brief Builds location metadata packet
// @param none
// @return Location metadata packet
fw_aircraft_metadata_t Packetizer::location_metadata()
{
    // Initialize packet and clear the needed memory
    fw_aircraft_metadata_t aircraft;
    memset(&aircraft, 0, sizeof(aircraft));
    
    // Prompt user for lat/long/height info (to save time)
    float tmp_float;
    int tmp_16;

    printf("Lattitude: ");
    scanf("%f", &tmp_float);
    aircraft.gpsLat = (int)(tmp_float * 1e7);

    printf("Longitude: ");
    scanf("%f", &tmp_float);
    aircraft.gpsLon = (int)(tmp_float * 1e7);

    printf("Altitude (m): ");
    scanf("%f", &aircraft.gpsAlt);

    // Aircraft attitude data.
    printf("Aircraft Roll (deg): ");
    scanf("%d", &tmp_16);
    aircraft.roll = (int16)(tmp_16 * (3.14159 / 180.0) * 1e4);

    printf("Aircraft Pitch (deg): ");
    scanf("%d", &tmp_16);
    aircraft.pitch = (int16)(tmp_16 * (3.14159 / 180.0) * 1e4);

    printf("Aircraft Yaw (deg): ");
    scanf("%d", &tmp_16);
    aircraft.yaw = (int16)(tmp_16 * (3.14159 / 180.0) * 1e4);

    // Payload attitude data.
    printf("Payload Roll (deg): ");
    scanf("%d", &tmp_16);
    aircraft.payloadRoll = (int16)(tmp_16 * (3.14159 / 180.0) * 1e4);

    printf("Payload Pitch (deg): ");
    scanf("%d", &tmp_16);
    aircraft.payloadEl = (int16)(tmp_16 * (3.14159 / 180.0) * 1e4);

    printf("Payload Yaw (deg): ");
    scanf("%d", &tmp_16);
    aircraft.payloadAz = (int16)(tmp_16 * (3.14159 / 180.0) * 1e4);

    // AP Mode 4 required for terrain estimate to trigger
    aircraft.apMode = 4;
    
    // Return the constructed packet
    return aircraft;
}

// @brief Builds video session packet
// @param none
// @return Video session packet
fw_video_session_t Packetizer::video()
{
    // Initialize packet and clear the needed memory
    fw_video_session_t video_session;
    memset(&video_session, 0, sizeof(video_session));
    
    // Initialize the session command element according to user input
    uint32 tmp_u32;
    printf("open (0): ");
    scanf("%u", &tmp_u32);
    video_session.sessionCmd = tmp_u32 & 0xFF;

    if (video_session.sessionCmd == 0)
    {
        // Prompt user for exposure time
        printf("Exposure Time (us): ");
        scanf("%u", &video_session.exposureUs);
    }

    // Return the constructed packet
    return video_session;
}

// @brief Builds video session w/timestamp packet
// @param currTv Current time value
// @return Video session w/timestamp packet
fw_video_session_t Packetizer::videotime(struct timeval &currTv)
{
    // Initiailize the packet and clear the needed memory
    fw_video_session_t video_session;
    memset(&video_session, 0, sizeof(video_session));
    
    // Prompt user to open session
    uint32 tmp_u32;
    printf("open (0): ");
    scanf("%u", &tmp_u32);

    // Initialize the session command element according to user input
    video_session.sessionCmd = tmp_u32 & 0xFF;

    if (video_session.sessionCmd == 0)
    {
        // Initialize exposure time according to user input
        printf("Exposure Time (us): ");
        scanf("%u", &video_session.exposureUs);

        // Update timestamp element with current time of day
        memset(&currTv, 0, sizeof(currTv));
        gettimeofday(&currTv, NULL);
        video_session.timeStamp = (((unsigned long long)currTv.tv_sec) * 1000000) + ((unsigned long long)currTv.tv_usec);
        printf("Timestamp: %llu\n", video_session.timeStamp);
    }

    // Return the constructed packet
    return video_session;
}

// @brief Builds advanced video session packet
// @param currTv Current time value
// @return Advanced video session packet
fw_video_session_advanced_t Packetizer::videoadvanced(struct timeval &currTv)
{
    // Initialize packet and clear the needed memory
    fw_video_session_advanced_t video_session_adv;
    memset(&video_session_adv, 0, sizeof(video_session_adv));
    
    // Initialize the session command element according to user input
    uint32 tmp_u32;
    printf("open (0): ");
    scanf("%u", &tmp_u32);
    video_session_adv.sessionCmd = tmp_u32 & 0xFF;

    if (video_session_adv.sessionCmd == 0)
    {
        // Initialize exposure time from user input
        printf("Exposure Time (us, use 0 for auto): ");
        scanf("%u", &video_session_adv.exposureUs);

        // Initiailize bitrate from user input
        printf("Bitrate (bps[100,000:10,000,000): ");
        scanf("%u", &video_session_adv.bitrate);

        // Initialize group of pictures from user input
        printf("Group of Pictures (GOP, [1:30]): ");
        scanf("%u", &tmp_u32);
        video_session_adv.gop = tmp_u32;

        // Initiailize metadata source from user input
        printf("Metadata Source (0=none, 1=Sentera ICD, 2=Dyn Test, 3=Stat Test): ");
        scanf("%u", &tmp_u32);
        video_session_adv.metadataSource = tmp_u32;

        // Toggle estab element from user input
        printf("Estab (0=disabled, 1=enabled): ");
        scanf("%u", &tmp_u32);
        video_session_adv.eStab = tmp_u32;

        // Initialize auto exposure target value from user input
        printf("Auto Exposure Target [1:4095] (rec=1024): ");
        scanf("%u", &tmp_u32);
        video_session_adv.aeTarget = tmp_u32;

        // Initialize gain value from user input
        printf("Gain: [1:255] (rec=64): ");
        scanf("%u", &tmp_u32);
        video_session_adv.gain = tmp_u32;

        // Initialize horizontal resolution from user input
        printf("Horizontal Resolution [128:1248]:");
        scanf("%u", &tmp_u32);
        video_session_adv.hResolution = tmp_u32;

        // Initialize vertical resolution from user input
        printf("Vertical Resolution [128:720]: ");
        scanf("%u", &tmp_u32);
        video_session_adv.vResolution = tmp_u32;

        // Initialize timestamp with current time of day
        memset(&currTv, 0, sizeof(currTv));
        gettimeofday(&currTv, NULL);
        video_session_adv.timeStamp = (((unsigned long long)currTv.tv_sec) * 1000000) + ((unsigned long long)currTv.tv_usec);
        printf("Timestamp: %llu\n", video_session_adv.timeStamp);
    }

    // Return the constructed packet
    return video_session_adv;
}

// @brief Builds exposure adjustment packet
// @param none
// @return Exposure adjustment packet
fw_exposure_adjust_t Packetizer::exposureadjust()
{
    // Initialize packet and clear the needed memory
    fw_exposure_adjust_t exposure_adjust;
    memset(&exposure_adjust, 0, sizeof(exposure_adjust));
    
    // Initialize exposure time from user input
    uint32 tmp_u32;
    printf("Exposure: ");
    scanf("%u", &tmp_u32);
    exposure_adjust.exposure_time_us = tmp_u32;

    // Initialize analog gain from user input
    printf("Analog Gain: [0:3] (0=1x, 1=2x, 2=4x, 3=8x): ");
    scanf("%u", &tmp_u32);
    exposure_adjust.analog_gain = tmp_u32;

    // Initialize digital gain from user input
    printf("Digital Gain: [0:255] (32=1x): ");
    scanf("%u", &tmp_u32);
    exposure_adjust.digital_gain = tmp_u32;

    // Return the constructed packet
    return exposure_adjust;
}

// @brief Builds focus packet
// @param none
// @return Focus packet
fw_focus_session_t Packetizer::focus()
{
    // Initialize packet and clear the needed memory
    fw_focus_session_t focus_session;
    memset(&focus_session, 0, sizeof(focus_session));
    
    // Initialize session command element from user input
    uint32 tmp_u32;
    printf("open (0): ");
    scanf("%u", &tmp_u32);
    focus_session.sessionCmd = tmp_u32 & 0xFF;

    if (focus_session.sessionCmd == 0)
    {
        // Initialize exposure time from user input
        printf("Exposure Time (us) - 0=auto: ");
        scanf("%u", &focus_session.exposureUs);
    }
    
    // Return the constructed packet
    return focus_session;
}

// @brief Builds still focus packet
// @param none
// @return Still focus packet
fw_still_focus_session_t Packetizer::sf()
{
    // Initialize the packet and clear the needed memory
    fw_still_focus_session_t still_focus_session;
    memset(&still_focus_session, 0, sizeof(still_focus_session));

    // Initialize exposure time from user input
    printf("Exposure Time (us) - 0=auto: ");
    scanf("%u", &still_focus_session.exposureUs);

    // Return the constructed packet
    return still_focus_session;
}

// @brief Builds spoof packet
// @param trigger_mask Specifies which sensors are involved
// @return Spoof packet
fw_imager_trigger_t Packetizer::spoof(uint8 trigger_mask)
{
    // Inialize packet and clear the needed memory
    fw_imager_trigger_t imager_trigger;
    memset(&imager_trigger, 0, sizeof(imager_trigger));
    
    // Set packet elements for spoofing
    imager_trigger.imgSelect = trigger_mask; 
    imager_trigger.trigID = 0x77;
    imager_trigger.trigMode = 1;
    imager_trigger.trigPeriod = 0;

    // Return constructed packet
    return imager_trigger;
}

fw_system_time_t Packetizer::system_time()
{
    fw_system_time_t system_time;
    memset(&system_time, 0, sizeof(system_time));
    uint32 tmp_u32;
    uint64_t tmp_u64;

    printf("Command [ Get(0), Set(1) ]: ");
    scanf("%u", &tmp_u32);
    system_time.command = tmp_u32 & 0xFF;

    // Get a random session ID
    system_time.requestID = rand() & 0xFFFF;

    if (system_time.command == SYSTIME_SET) 
    {
        // Ask the user for the current time (for testing)
        printf("Current time (us): ");
        scanf("%" SCNu64, &tmp_u64);
        system_time.timeStamp = tmp_u64;
    } 
    else
    {
        system_time.timeStamp = 0;
    }

    printf("\nTime packet ID: %u \n", system_time.requestID);
    return system_time;
}

fw_video_adjust_relative_t Packetizer::video_adjust_relative(uint8 trigger_mask)
{
    fw_video_adjust_relative_t adjust;
    memset(&adjust, 0, sizeof(adjust));
    uint32 tmp_u32;

    // Get the requested EV and ISO change
    printf("EV Adjust [None(0), Dec(1), Inc(2), Dec Loop(3), Inc Loop(4) ]:");
    scanf("%u", &tmp_u32);
    adjust.evCommand = tmp_u32 & 0xFF;

    printf("ISO Adjust [None(0), Dec(1), Inc(2), Dec Loop(3), Inc Loop(4) ]:");
    scanf("%u", &tmp_u32);
    adjust.isoCommand = tmp_u32 & 0xFF;

    // Initialize imager select element from trigger mask
    adjust.imgSelect = trigger_mask;

    // Return the constructed packet
    return adjust;
}