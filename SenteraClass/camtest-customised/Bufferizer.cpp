
// includes
#include "Bufferizer.h"

const unsigned char crc8_7_table[] = {
	0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,
	0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
	0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,
	0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
	0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,
	0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
	0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,
	0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
	0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,
	0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
	0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,
	0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
	0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,
	0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
	0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,
	0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
	0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,
	0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
	0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,
	0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
	0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,
	0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
	0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,
	0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
	0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,
	0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
	0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,
	0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
	0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,
	0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
	0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,
	0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};

// @brief Error-detection built into final byte of constructed commands
// @param cur_crc Current CRC value
// @param msg Pointer to buffer
// @return Length of message/buffer
uint8_t Bufferizer::CRC8_7(uint8_t cur_crc, uint8_t *msg, uint32_t msg_size)
{
    uint8_t crc = cur_crc;
    uint32_t i;

    for (i = 0; i < msg_size; i++)
    {
        crc = crc8_7_table[crc ^ msg[i]];
    }
    return crc;
}

// =============================================================================
// *****************************************************************************
// ************************** Public Static Functions **************************
// *****************************************************************************
// @brief Build a still image session open command (or a common session close/kill command)
// @param session Imager session packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::session(fw_imager_session_t& session, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 142;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = IMAGER_SESSION;         n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = session.sessionCmd;     n++;
    for (i = 0; i < 128; i++)
    {
        buf[n] = session.sessionName[i]; n++;
    }

    buf[n] = session.utcYear >> 0;    n++;
    buf[n] = session.utcMonth >> 0;    n++;
    buf[n] = session.utcDay >> 0;    n++;
    buf[n] = session.utcHour >> 0;    n++;
    buf[n] = session.utcMinute >> 0;    n++;
    buf[n] = session.utcMillisecond & 0xFF;    n++;
    buf[n] = session.utcMillisecond >> 8;    n++;
    buf[n] = session.sessionID & 0xFF;    n++;
    buf[n] = session.sessionID >> 8;    n++;
    buf[n] = session.buildVersion & 0xFF;    n++;
    buf[n] = session.buildVersion >> 8;    n++;
    buf[n] = session.aircraftType >> 0;    n++;
    buf[n] = session.resumeSession >> 0;    n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;
    return n;
}

// @brief Build a video session open command
// @param video_session Video session packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::video(fw_video_session_t& video_session, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 5;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = VIDEO_SESSION;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = video_session.sessionCmd;               n++;
    buf[n] = video_session.exposureUs >> 0;    n++;
    buf[n] = video_session.exposureUs >> 8;    n++;
    buf[n] = video_session.exposureUs >> 16;    n++;
    buf[n] = video_session.exposureUs >> 24;    n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build a video session open command with a timestamp (ICD rev K)
// @param video_session Video session packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::videotime(fw_video_session_t& video_session, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 13;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = VIDEO_SESSION;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = video_session.sessionCmd;               n++;
    buf[n] = video_session.exposureUs >> 0;    n++;
    buf[n] = video_session.exposureUs >> 8;    n++;
    buf[n] = video_session.exposureUs >> 16;    n++;
    buf[n] = video_session.exposureUs >> 24;    n++;

    buf[n] = video_session.timeStamp >> 0;    n++;
    buf[n] = video_session.timeStamp >> 8;    n++;
    buf[n] = video_session.timeStamp >> 16;    n++;
    buf[n] = video_session.timeStamp >> 24;    n++;
    buf[n] = video_session.timeStamp >> 32;    n++;
    buf[n] = video_session.timeStamp >> 40;    n++;
    buf[n] = video_session.timeStamp >> 48;    n++;
    buf[n] = video_session.timeStamp >> 56;    n++;

    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build an advanced video session open command
// @param video_session_adv Advanced video session packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::videoadvanced(fw_video_session_advanced_t& video_session_adv, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 28;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = VIDEO_SESSION_ADVANCED;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = video_session_adv.sessionCmd;               n++;
    buf[n] = video_session_adv.exposureUs >> 0;    n++;
    buf[n] = video_session_adv.exposureUs >> 8;    n++;
    buf[n] = video_session_adv.exposureUs >> 16;    n++;
    buf[n] = video_session_adv.exposureUs >> 24;    n++;

    buf[n] = video_session_adv.timeStamp >> 0;    n++;
    buf[n] = video_session_adv.timeStamp >> 8;    n++;
    buf[n] = video_session_adv.timeStamp >> 16;    n++;
    buf[n] = video_session_adv.timeStamp >> 24;    n++;
    buf[n] = video_session_adv.timeStamp >> 32;    n++;
    buf[n] = video_session_adv.timeStamp >> 40;    n++;
    buf[n] = video_session_adv.timeStamp >> 48;    n++;
    buf[n] = video_session_adv.timeStamp >> 56;    n++;

    buf[n] = video_session_adv.bitrate >> 0;    n++;
    buf[n] = video_session_adv.bitrate >> 8;    n++;
    buf[n] = video_session_adv.bitrate >> 16;    n++;
    buf[n] = video_session_adv.bitrate >> 24;    n++;

    buf[n] = video_session_adv.gop >> 0;    n++;

    buf[n] = video_session_adv.metadataSource;    n++;

    buf[n] = video_session_adv.eStab;             n++;

    buf[n] = video_session_adv.aeTarget >> 0;    n++;
    buf[n] = video_session_adv.aeTarget >> 8;    n++;

    buf[n] = video_session_adv.gain >> 0;    n++;
    buf[n] = video_session_adv.gain >> 8;    n++;

    buf[n] = video_session_adv.hResolution >> 0;    n++;
    buf[n] = video_session_adv.hResolution >> 8;    n++;

    buf[n] = video_session_adv.vResolution >> 0;    n++;
    buf[n] = video_session_adv.vResolution >> 8;    n++;

    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build a video adjust packet
// @param video_adjust Video adjust packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::videoadjust(fw_video_adjust_t& video_adjust, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 10;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = VIDEO_ADJUST;           n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = video_adjust.bitrate >> 0;    n++;
    buf[n] = video_adjust.bitrate >> 8;    n++;
    buf[n] = video_adjust.bitrate >> 16;    n++;
    buf[n] = video_adjust.bitrate >> 24;    n++;

    buf[n] = video_adjust.gop >> 0;    n++;

    buf[n] = video_adjust.eStab;             n++;

    buf[n] = video_adjust.aeTarget >> 0;    n++;
    buf[n] = video_adjust.aeTarget >> 8;    n++;

    buf[n] = video_adjust.gain >> 0;    n++;
    buf[n] = video_adjust.gain >> 8;    n++;


    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build an exposure adjust packet
// @param exposure_adjust Exposure adjust packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::exposureadjust(fw_exposure_adjust_t& exposure_adjust, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 12;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = EXPOSURE_ADJUST;           n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = exposure_adjust.exposure_time_us >> 0;    n++;
    buf[n] = exposure_adjust.exposure_time_us >> 8;    n++;
    buf[n] = exposure_adjust.exposure_time_us >> 16;    n++;
    buf[n] = exposure_adjust.exposure_time_us >> 24;    n++;

    buf[n] = exposure_adjust.analog_gain >> 0;    n++;
    buf[n] = exposure_adjust.analog_gain >> 8;    n++;
    buf[n] = exposure_adjust.analog_gain >> 16;    n++;
    buf[n] = exposure_adjust.analog_gain >> 24;    n++;

    buf[n] = exposure_adjust.digital_gain >> 0;    n++;
    buf[n] = exposure_adjust.digital_gain >> 8;    n++;
    buf[n] = exposure_adjust.digital_gain >> 16;    n++;
    buf[n] = exposure_adjust.digital_gain >> 24;    n++;

    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build a focus session open command
// @param focus Focus session packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::focus(fw_focus_session_t& focus, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 5;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = FOCUS_SESSION;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = focus.sessionCmd;               n++;
    buf[n] = focus.exposureUs >> 0;    n++;
    buf[n] = focus.exposureUs >> 8;    n++;
    buf[n] = focus.exposureUs >> 16;    n++;
    buf[n] = focus.exposureUs >> 24;    n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build a still focus session open command
// @param still_focus Still focus session packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::sf(fw_still_focus_session_t& still_focus, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 5;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = STILL_FOCUS_SESSION;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = still_focus.sessionCmd;               n++;
    buf[n] = still_focus.exposureUs >> 0;    n++;
    buf[n] = still_focus.exposureUs >> 8;    n++;
    buf[n] = still_focus.exposureUs >> 16;    n++;
    buf[n] = still_focus.exposureUs >> 24;    n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2);  n++;
    return n;
}

// @brief Build a still image trigger command
// @param trigger Imager trigger packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::trigger(fw_imager_trigger_t& trigger, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 6;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = IMAGER_TRIGGER;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = trigger.imgSelect;              n++;
    buf[n] = trigger.trigID >> 0;           n++;
    buf[n] = trigger.trigID >> 8;           n++;
    buf[n] = trigger.trigMode;               n++;
    buf[n] = trigger.trigPeriod >> 0;  n++;
    buf[n] = trigger.trigPeriod >> 8;  n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;

    return n;
}

// @brief Build an imager zoom command
// @param zoom Imager zoom packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::zoom(fw_imager_zoom_t& zoom, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 4;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = IMAGER_ZOOM;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = zoom.imgSelect;              n++;
    buf[n] = zoom.zoomMode;               n++;
    buf[n] = zoom.zoomRate;               n++;
    buf[n] = zoom.zoomSteps;               n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;

    return n;
}

// @brief Build an imager preview stream setup command
// @param preview Imager preview stream setup packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::preview_stream_setup(fw_imager_preview_stream_setup_t& preview, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 13;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = IMAGER_PREVIEW_STREAM_SETUP;                   n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = preview.imgSelect;              n++;
    buf[n] = preview.videoStatus;            n++;
    buf[n] = preview.videoDstIP >> 0;        n++;
    buf[n] = preview.videoDstIP >> 8;        n++;
    buf[n] = preview.videoDstIP >> 16;       n++;
    buf[n] = preview.videoDstIP >> 24;       n++;
    buf[n] = preview.videoDstPort >> 0;      n++;
    buf[n] = preview.videoDstPort >> 8;      n++;
    buf[n] = preview.cameraConfig >> 0;      n++;
    buf[n] = preview.cameraConfig >> 8;      n++;
    buf[n] = preview.cameraConfig >> 16;     n++;
    buf[n] = preview.cameraConfig >> 24;     n++;
    buf[n] = preview.overlayConfig;          n++;

    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;

    return n;
}

// @brief Build an elevation metadata command
// @param elevation Elevation metadata packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::elevation_metadata(fw_elevation_metadata_t& elevation, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 14;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = ELEVATION_METADATA;     n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = elevation.mode;         n++;
    buf[n] = elevation.dataValid;    n++;

    // memcpy may not be valid on all endian systems, but neither is the byte method above
    // We can either use a memcpy or treat the float as a pointer to an array of bytes via a cast
    // http://stackoverflow.com/questions/21005845/how-to-get-float-bytes
    memcpy(&buf[n], &elevation.cameraAGL, sizeof(float)); n += sizeof(float);
    memcpy(&buf[n], &elevation.terrainMSL, sizeof(float)); n += sizeof(float);
    memcpy(&buf[n], &elevation.cameraMSL, sizeof(float)); n += sizeof(float);

    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;

    return n;
}

// @brief Build an aircraft metadata packet for spoofing an autopilot
// @param aircraft Aircraft metadata packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::aircraft_metadata(fw_aircraft_metadata_t& aircraft, uint8_t *buf)
{

    int i = 0;
    int n = 0;
    int k = 0;
    int length = 87;

    buf[n] = 'F';                    n++;
    buf[n] = 'W';                    n++;
    buf[n] = AIRCRAFT_METADATA;     n++;
    buf[n] = length >> 0;    n++;
    buf[n] = length >> 8;    n++;

    buf[n] = aircraft.agentID >> 0;      n++;
    buf[n] = aircraft.agentID >> 8;      n++;
    buf[n] = aircraft.sessionTime >> 0;  n++;
    buf[n] = aircraft.sessionTime >> 8;  n++;
    buf[n] = aircraft.sessionTime >> 16; n++;
    buf[n] = aircraft.sessionTime >> 24; n++;
    buf[n] = aircraft.gpsFixType >> 0;   n++;
    buf[n] = aircraft.gpsSVs >> 0;       n++;
    buf[n] = aircraft.gpsLat >> 0;       n++;
    buf[n] = aircraft.gpsLat >> 8;       n++;
    buf[n] = aircraft.gpsLat >> 16;      n++;
    buf[n] = aircraft.gpsLat >> 24;      n++;
    buf[n] = aircraft.gpsLon >> 0;       n++;
    buf[n] = aircraft.gpsLon >> 8;       n++;
    buf[n] = aircraft.gpsLon >> 16;      n++;
    buf[n] = aircraft.gpsLon >> 24;      n++;

    // 16 bytes

    memcpy(&buf[n], &aircraft.gpsAlt, sizeof(float)); n += sizeof(float);
    memcpy(&buf[n], &aircraft.gpsVNorth, sizeof(float)); n += sizeof(float);
    memcpy(&buf[n], &aircraft.gpsVEast, sizeof(float)); n += sizeof(float);
    memcpy(&buf[n], &aircraft.gpsVDown, sizeof(float)); n += sizeof(float);

    // 32 bytes

    buf[n] = aircraft.roll >> 0;              n++;
    buf[n] = aircraft.roll >> 8;              n++;
    buf[n] = aircraft.pitch >> 0;             n++;
    buf[n] = aircraft.pitch >> 8;             n++;
    buf[n] = aircraft.yaw >> 0;               n++;
    buf[n] = aircraft.yaw >> 8;               n++;
    buf[n] = aircraft.payloadRoll >> 0;       n++;
    buf[n] = aircraft.payloadRoll >> 8;       n++;
    buf[n] = aircraft.payloadEl >> 0;         n++;
    buf[n] = aircraft.payloadEl >> 8;         n++;
    buf[n] = aircraft.payloadAz >> 0;         n++;
    buf[n] = aircraft.payloadAz >> 8;         n++;
    buf[n] = aircraft.payloadRollRate >> 0;   n++;
    buf[n] = aircraft.payloadRollRate >> 8;   n++;
    buf[n] = aircraft.payloadElRate >> 0;     n++;
    buf[n] = aircraft.payloadElRate >> 8;     n++;
    buf[n] = aircraft.payloadAzRate >> 0;     n++;
    buf[n] = aircraft.payloadAzRate >> 8;     n++;

    // 50 bytes

    buf[n] = aircraft.xGyro >> 0;       n++;
    buf[n] = aircraft.xGyro >> 8;       n++;
    buf[n] = aircraft.yGyro >> 0;       n++;
    buf[n] = aircraft.yGyro >> 8;       n++;
    buf[n] = aircraft.zGyro >> 0;       n++;
    buf[n] = aircraft.zGyro >> 8;       n++;
    buf[n] = aircraft.xAccel >> 0;      n++;
    buf[n] = aircraft.xAccel >> 8;      n++;
    buf[n] = aircraft.yAccel >> 0;      n++;
    buf[n] = aircraft.yAccel >> 8;      n++;
    buf[n] = aircraft.zAccel >> 0;      n++;
    buf[n] = aircraft.zAccel >> 8;      n++;
    buf[n] = aircraft.xMag >> 0;        n++;
    buf[n] = aircraft.xMag >> 8;        n++;
    buf[n] = aircraft.yMag >> 0;        n++;
    buf[n] = aircraft.yMag >> 8;        n++;
    buf[n] = aircraft.zMag >> 0;        n++;
    buf[n] = aircraft.zMag >> 8;        n++;

    // 68 bytes

    buf[n] = aircraft.ias >> 0;         n++;
    buf[n] = aircraft.ias >> 8;         n++;
    buf[n] = aircraft.windDir >> 0;     n++;
    buf[n] = aircraft.windDir >> 8;     n++;
    buf[n] = aircraft.windMag >> 0;     n++;
    buf[n] = aircraft.windMag >> 8;     n++;

    // 74 bytes

    buf[n] = aircraft.apMode >> 0;           n++;
    buf[n] = aircraft.waypointType >> 0;     n++;
    buf[n] = aircraft.waypointNum >> 0;      n++;
    buf[n] = aircraft.gpsDiscardCount >> 0;  n++;
    buf[n] = aircraft.gpsDiscardCount >> 8;  n++;

    // 79 bytes

    memcpy(&buf[n], &aircraft.baro, sizeof(float)); n += sizeof(float);
    memcpy(&buf[n], &aircraft.agl, sizeof(float)); n += sizeof(float);

    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;
    return n;

}

// @brief Build a System time packet
// @param time System time packet
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::system_time(fw_system_time_t& time, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 18;

    buf[n] = 'F';                   n++;
    buf[n] = 'W';                   n++;
    buf[n] = SYSTEM_TIME;           n++;
    buf[n] = length >> 0;           n++;
    buf[n] = length >> 8;           n++;

    buf[n] = time.command;          n++;
    buf[n] = time.requestID >> 0;   n++;
    buf[n] = time.requestID >> 8;   n++;
    buf[n] = time.timeStamp >> 0;   n++;
    buf[n] = time.timeStamp >> 8;   n++;
    buf[n] = time.timeStamp >> 16;  n++;
    buf[n] = time.timeStamp >> 24;  n++;
    buf[n] = time.timeStamp >> 32;  n++;
    buf[n] = time.timeStamp >> 40;  n++;
    buf[n] = time.timeStamp >> 48;  n++;
    buf[n] = time.timeStamp >> 56;  n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;

    return n;
}

// @brief Build an imager relative video adjust packet 
// @param adjust Imager Relative Adjust Packet 
// @param buf Pointer to buffer
// @return Length of initialized buffer space
int Bufferizer::video_adjust_relative(fw_video_adjust_relative_t& adjust, uint8_t *buf)
{
    int i = 0;
    int n = 0;
    int k = 0;
    int length = 3;

    buf[n] = 'F';                   n++;
    buf[n] = 'W';                   n++;
    buf[n] = VIDEO_ADJUST_RELATIVE; n++;
    buf[n] = length >> 0;           n++;
    buf[n] = length >> 8;           n++;

    buf[n] = adjust.imgSelect;      n++;
    buf[n] = adjust.evCommand;      n++;
    buf[n] = adjust.isoCommand;     n++;
    buf[n] = Bufferizer::CRC8_7(0, &buf[2], n - 2); n++;

    return n;
}

// *****************************************************************************
// ************************** Protected Static Functions ***********************
// *****************************************************************************

// *****************************************************************************
// ************************** Private Static Functions *************************
// *****************************************************************************