
// includes
#include "DataPacketizer.h"

// *****************************************************************************
// ********************************* DEFINES ***********************************
// *****************************************************************************


// *****************************************************************************
// ************************** Public Static Functions **************************
// *****************************************************************************
// @brief Builds session packet
// @param none
// @return Imager session packet
fw_imager_session_t DataPacketizer::session(uint8_t command, const char *name)
{
    // Initialize packet and clear the needed memory
    fw_imager_session_t imager_session;
    memset(&imager_session, 0, sizeof(imager_session));

    // Open, close, or kill session according to user input
    imager_session.sessionCmd = command & 0xFF; //open (0), close (1), or kill (2):

    if (imager_session.sessionCmd == 0)
    {
		for (int i = 0; i < 128; i++) {
			imager_session.sessionName[i] = (uint8_t)name[i];
		}

        imager_session.resumeSession = 0x1 & 0xF;

		// Add UTC time data to the session open packet.
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

        imager_session.sessionID = 1; // Use fixed session ID for testing.
    }

    // Return the constructed packet
    return imager_session;
}

// @brief Builds trigger command packet
// @param trigger_mask Specifies which sensors are involved
// @return Imager trigger packet
fw_imager_trigger_t DataPacketizer::trigger(uint8_t trigger_mask, uint8_t option, uint32_t interval)
{
    // Initialize packet and clear the needed memory
    fw_imager_trigger_t imager_trigger;
    memset(&imager_trigger, 0, sizeof(imager_trigger));
    
    // Initialize trigger mode from user input
    imager_trigger.trigMode = option & 0xFF; //Disable (0), Single (1), or Continuous (2)

    if (imager_trigger.trigMode == 2)
    {
        imager_trigger.trigPeriod = interval & 0xFFFF; //Interval (ms)
    }

    imager_trigger.imgSelect = trigger_mask;    // Initialize imager select element from trigger mask
    imager_trigger.trigID = 0x77;    // Initialize trigger ID element

    /// Return the constructed packet
    return imager_trigger;
}

// @brief Builds zoom packet
// @param trigger_mask Specifies which sensors are involved
// @return Zoom packet
fw_imager_zoom_t DataPacketizer::zoom(uint8_t trigger_mask)
{
    // Initialize packet and clear the needed memory
    fw_imager_zoom_t imager_zoom;
    memset(&imager_zoom, 0, sizeof(imager_zoom));
   
    imager_zoom.zoomMode = 0x02 & 0xFF; //Mode - Rate (1), Steps (2). --- should stay Steps 

    // Initialize value for rate or steps from user input as applicable
    if (imager_zoom.zoomMode == 1)
    {
        imager_zoom.zoomRate = 0x00 & 0xFF;			// Rate
    }
    else if (imager_zoom.zoomMode == 2)
    {
        imager_zoom.zoomSteps = 0x01 & 0xFF;		// number of steps
    }

    imager_zoom.imgSelect = trigger_mask;		    // Initialize imager select element from trigger mask

    // Return the constructed packet
    return imager_zoom;
}

// @brief Builds preview stream setup packet
// @param trigger_mask Specifies which sensors are involved
// @return Preview stream setup packet
fw_imager_preview_stream_setup_t DataPacketizer::preview_stream_setup(uint8_t trigger_mask)
{
    // Initialize packet and clear the needed memeory
    fw_imager_preview_stream_setup_t imager_preview;
    memset(&imager_preview, 0, sizeof(imager_preview));
    
    imager_preview.videoStatus = 0x01 & 0xFF; //Enable(1), Disable(0)

    // Prompt user for stream information
    in_addr_t dest_in_addr;
    if (imager_preview.videoStatus == 1)
    {
        // Initialize destination IP element 
        dest_in_addr = inet_addr("0"); //0 indicates default/previous IP
        imager_preview.videoDstIP = ntohl((uint32_t)dest_in_addr);
        printf("Dest IP: %x\n", imager_preview.videoDstIP);

        // Initialize destination port element 
        uint16_t port_int = 0; // default port (8080)
        imager_preview.videoDstPort = port_int & 0xFFFF;

        // Read in preview options for camera 0
        imager_preview.cameraConfig += ((0x02 & 0xFF) << 0);    //Cam 0 Pos [NoChange(0), Disable(1), Full(2), PIP(3), Top(4), Bottom(5), Overlay(6)]
        imager_preview.cameraConfig += ((0x02 & 0xFF) << 8);	//Cam 0 Opt[NoChange(0), Normal(1), Live NDVI(2)]

        // Read in preview options for camera 1
        imager_preview.cameraConfig += ((0x02 & 0xFF) << 16); //Cam 1 Pos [NoChange(0), Disable(1), Full(2), PIP(3), Top(4), Bottom(5), Overlay(6)]
        imager_preview.cameraConfig += ((0x02 & 0xFF) << 24); //Cam 1 Opt [NoChange(0), Normal(1), Live NDVI(2)]

        imager_preview.overlayConfig = (0x02 & 0xFF); //Overlay NoChange(0), Disable(1), Enable(2)
    }

    imager_preview.imgSelect = trigger_mask;     // Initialize imager select element from trigger mask

    // Return the constructed packet
    return imager_preview;
}

// @brief Builds elevation metadata packet
// @param none
// @return Elevation metadata packet
fw_elevation_metadata_t DataPacketizer::elevation_metadata()
{
    // Initialize packet and clear the needed memeory
    fw_elevation_metadata_t elevation;
    memset(&elevation, 0, sizeof(elevation));

    // Initialize elevation mode from user input   
    elevation.mode = 0x00 & 0xFF; //Mode - Camera AGL (0), Terrain MSL (1), Camera MSL (2)
	elevation.cameraAGL = 2; //Camera AGL(m)
	elevation.terrainMSL = 0;
    elevation.cameraMSL = 0;

	return elevation;
}

// @brief Builds location metadata packet
// @param none
// @return Location metadata packet
fw_aircraft_metadata_t DataPacketizer::location_metadata()
{
    // Initialize packet and clear the needed memory
    fw_aircraft_metadata_t aircraft;
    memset(&aircraft, 0, sizeof(aircraft));
    
	float tmp_float = 0;
	int tmp_16 = 0; // set later? for now everything is 0

    aircraft.gpsLat = (int)(tmp_float * 1e7);		//Lattitude
    aircraft.gpsLon = (int)(tmp_float * 1e7);		//Longitude
	aircraft.gpsAlt = 0; 							//Altitude (m)

    // Aircraft attitude data.
    aircraft.roll = (int16_t)(tmp_16 * (3.14159 / 180.0) * 1e4);		//Aircraft roll (deg)
    aircraft.pitch = (int16_t)(tmp_16 * (3.14159 / 180.0) * 1e4);		//Aircraft pitch (deg)
    aircraft.yaw = (int16_t)(tmp_16 * (3.14159 / 180.0) * 1e4);		//Aircraft yaw (deg)

    // Payload attitude data.
    aircraft.payloadRoll = (int16_t)(tmp_16 * (3.14159 / 180.0) * 1e4);	//Payload Roll (deg)
    aircraft.payloadEl = (int16_t)(tmp_16 * (3.14159 / 180.0) * 1e4);		//Payload Pitch (deg)
    aircraft.payloadAz = (int16_t)(tmp_16 * (3.14159 / 180.0) * 1e4);		//Payload Yaw (deg)

    aircraft.apMode = 4;		    // AP Mode 4 required for terrain estimate to trigger
    
    // Return the constructed packet
    return aircraft;
}

// @brief Builds video session packet
// @param none
// @return Video session packet
fw_video_session_t DataPacketizer::video()
{
    // Initialize packet and clear the needed memory
    fw_video_session_t video_session;
    memset(&video_session, 0, sizeof(video_session));
    
    video_session.sessionCmd = 0x00 & 0xFF; // Open (0)

    if (video_session.sessionCmd == 0)
    {
		video_session.exposureUs = 0; // Exposure Time(us) (0 = Autoexposure)
    }

    // Return the constructed packet
    return video_session;
}

// @brief Builds video session w/timestamp packet
// @param currTv Current time value
// @return Video session w/timestamp packet
fw_video_session_t DataPacketizer::videotime(struct timeval &currTv)
{
    // Initiailize the packet and clear the needed memory
    fw_video_session_t video_session;
    memset(&video_session, 0, sizeof(video_session));

    // Initialize the session command element according to user input
    video_session.sessionCmd = 0x00 & 0xFF;		// Open (0)

    if (video_session.sessionCmd == 0)
    {
		video_session.exposureUs = 0;			// Exposure Time(us) (0 = Autoexposure)

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
fw_video_session_advanced_t DataPacketizer::videoadvanced(struct timeval &currTv)
{
    // Initialize packet and clear the needed memory
    fw_video_session_advanced_t video_session_adv;
    memset(&video_session_adv, 0, sizeof(video_session_adv));
    
    video_session_adv.sessionCmd = 0x00 & 0xFF;			// Open (0)

    if (video_session_adv.sessionCmd == 0)
    {
        // Initialize exposure time from user input
		video_session_adv.exposureUs = 0;				//Exposure Time (us, use 0 for auto)
		video_session_adv.bitrate = 1000000;			// Bitrate (bps[100,000:10,000,000)
        video_session_adv.gop = 20;						//Group of Pictures (GOP, [1:30])
        video_session_adv.metadataSource = 0;			//Metadata Source (0=none, 1=Sentera ICD, 2=Dyn Test, 3=Stat Test)
        video_session_adv.eStab = 0;					//Estab (0=disabled, 1=enabled)
        video_session_adv.aeTarget = 1024;				//Auto Exposure Target [1:4095] (rec=1024)
        video_session_adv.gain = 64;					//Gain: [1:255] (rec=64)
        video_session_adv.hResolution = 512;			//Horizontal Resolution[128:1248]
        video_session_adv.vResolution = 384;			//Vertical Resolution[128:720]

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
fw_exposure_adjust_t DataPacketizer::exposureadjust()
{
    // Initialize packet and clear the needed memory
    fw_exposure_adjust_t exposure_adjust;
    memset(&exposure_adjust, 0, sizeof(exposure_adjust));
    
	exposure_adjust.exposure_time_us = 0x00; // Exposue time
    exposure_adjust.analog_gain = 0x01; //Analog Gain: [0:3] (0=1x, 1=2x, 2=4x, 3=8x)
    exposure_adjust.digital_gain = 32; //Digital Gain: [0:255] (32=1x)
    exposure_adjust.digital_gain = 32; //Digital Gain: [0:255] (32=1x)

    // Return the constructed packet
    return exposure_adjust;
}

// @brief Builds focus packet
// @param none
// @return Focus packet
fw_focus_session_t DataPacketizer::focus()
{
    // Initialize packet and clear the needed memory
    fw_focus_session_t focus_session;
    memset(&focus_session, 0, sizeof(focus_session));
    
    focus_session.sessionCmd = 0x00 & 0xFF;			//Open (0)

    if (focus_session.sessionCmd == 0)
    {
		focus_session.exposureUs = 0;				//Exposure Time (us) - 0=auto
    }

    // Return the constructed packet
    return focus_session;
}

// @brief Builds still focus packet
// @param none
// @return Still focus packet
fw_still_focus_session_t DataPacketizer::sf()
{
    // Initialize the packet and clear the needed memory
    fw_still_focus_session_t still_focus_session;
    memset(&still_focus_session, 0, sizeof(still_focus_session));

    still_focus_session.exposureUs = 0; //Exposure Time (us) - 0=auto

    // Return the constructed packet
    return still_focus_session;
}

// @brief Builds spoof packet
// @param trigger_mask Specifies which sensors are involved
// @return Spoof packet
fw_imager_trigger_t DataPacketizer::spoof(uint8_t trigger_mask)
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

fw_video_adjust_t DataPacketizer::video_adjust()
{
	// Inialize packet and clear the needed memory
	fw_video_adjust_t video_adjust;
	memset(&video_adjust, 0, sizeof(video_adjust));

	// Set packet elements for spoofing
	video_adjust.aeTarget = 1024;
	video_adjust.bitrate = 2000000;
	video_adjust.eStab = 0;
	video_adjust.gain = 64;
	video_adjust.gop = 30;

	// Return constructed packet
	return video_adjust;
}

fw_video_adjust_relative_t DataPacketizer::video_adjust_relative(uint8_t trigger_mask)
{
    fw_video_adjust_relative_t adjust;
    memset(&adjust, 0, sizeof(adjust));

    adjust.evCommand = 0x00 & 0xFF;			//EV Adjust [None(0), Dec(1), Inc(2), Dec Loop(3), Inc Loop(4) ]
    adjust.isoCommand = 0x00 & 0xFF;		//ISO Adjust [None(0), Dec(1), Inc(2), Dec Loop(3), Inc Loop(4) ]

    // Initialize imager select element from trigger mask
    adjust.imgSelect = trigger_mask;

    // Return the constructed packet
    return adjust;
}