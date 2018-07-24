/*
 * packets.h
 *
 *  Created on: Jun 16, 2015
 *      Author: Andrew Muehlfeld
 */

#ifndef PACKETS_H_
#define PACKETS_H_

#include <sys/types.h>

#define FW_HEADER0 0x46
#define FW_HEADER1 0x57
#define CRC_START_INDEX 2
#define MAX_TX_BUF_SIZE 64

// Full packet, including header and payload
typedef struct {
    uint8_t  header0;
    uint8_t  header1;
    uint8_t  type;
    uint16_t length;
    uint8_t  payload[256];
    uint8_t  crc;
} fw_packet_t;

// Autopilot Stages
typedef enum {
    PREFLIGHT = 0,
    DISARMED = 1,
    SPOOL_UP = 2,
    MANUAL_READY = 3,
    AUTO_READY = 4,
    CLIMB_OUT = 5,
    AIRBORNE_NORMAL = 6,
    AIRBORNE_NO_GPS = 7,
    LANDING = 8,
    POST_FLIGHT = 9,
    GROUND_RC_MODE = 10,
    AIRBORNE_RC_MODE = 11,
} ap_mode_e;

// "FW" packet types. ---------------------------------------------------------
typedef enum {
    AIRCRAFT_METADATA           = 0x01,  // To payload.
    IMAGER_TRIGGER              = 0x02,  // To payload.
    IMAGER_POWER                = 0x03,  // To payload.
    IMAGER_SESSION              = 0x04,  // To payload.
    VIDEO_SESSION               = 0x05,  // To payload.
    FOCUS_SESSION               = 0x06,  // To payload.
    STILL_FOCUS_SESSION         = 0x07,  // To payload.
    VIDEO_SESSION_ADVANCED      = 0x09,  // To payload.
    VIDEO_ADJUST                = 0x0A,  // To payload.
    EXPOSURE_ADJUST             = 0x0B,  // Bidirectional
    IMAGER_ZOOM                 = 0x0C,  // To payload.
    IMAGER_PREVIEW_STREAM_SETUP = 0x0D,  // To payload.
    ELEVATION_METADATA          = 0x0E,  // To payload.
    SYSTEM_TIME                 = 0x0F,  // To payload.
    VIDEO_ADJUST_RELATIVE       = 0x10,  // To payload.
    PAYLOAD_METADATA            = 0x81,  // From payload.
    IMAGER_TRIGGER_ACK          = 0x82,  // From payload.
    IMAGER_SESSION_ACK          = 0x83,  // From payload.
    SYSTEM_TIME_ACK             = 0x8F,  // From payload.
    FOCUS_SESSION_INFO          = 0xD0,  // From payload to laptop (not autopilot)
    PAYLOAD_EXCEPTION           = 0xFF,  // From payload.
} fw_packet_type_e;

// Payload exception packet definition. ---------------------------------------
typedef struct {
    uint32_t exception;      // TBD.
} fw_payload_exception_t;

typedef enum {
    PAYLOAD_EXCEPTION_SESSION      = 0x01,
    PAYLOAD_EXCEPTION_APPLICATION  = 0x02,
    PAYLOAD_EXCEPTION_SD_CARD_FULL = 0x03,
    PAYLOAD_EXCEPTION_MISSING_ADAPTIVE_SCOUTING_FILES = 0x04,
} fw_payload_exception_e;

// Aircraft metadata packet definition. ---------------------------------------

typedef struct {
    uint16_t agentID;        // Autopilot agent ID.
    uint32_t sessionTime;    // Milliseconds since start of session.
    uint8_t  gpsFixType;     // GPS fix type.
    uint8_t  gpsSVs;         // GPS number of SVs used in navigation solution.
    int32_t  gpsLon;         // GPS longitude.
    int32_t  gpsLat;         // GPS latitude.
    float   gpsAlt;         // GPS altitude MSL.
    float   gpsVNorth;      // GPS north velocity.
    float   gpsVEast;       // GPS east velocity.
    float   gpsVDown;       // GPS down velocity.
    int16_t  roll;           // Aircraft roll (X) angle.
    int16_t  pitch;          // Aircraft pitch (Y) angle.
    int16_t yaw;             // Aircraft yaw (Z) angle.
    int16_t  payloadRoll;    // Payload roll (X) angle.
    int16_t  payloadEl;      // Payload elevation (Y) angle.
    int16_t payloadAz;       // Payload azimuth (Z) angle.
    int16_t  payloadRollRate;// Payload roll (X) angular rate.
    int16_t  payloadElRate;  // Payload elevation (Y) angular rate.
    int16_t  payloadAzRate;  // Payload azimuth (Z) angular rate.
    int16_t  xGyro;          // X gyroscope.
    int16_t  yGyro;          // Y gyroscope.
    int16_t  zGyro;          // Z gyroscope.
    int16_t  xAccel;         // X accelerometer.
    int16_t  yAccel;         // Y accelerometer.
    int16_t  zAccel;         // Z accelerometer.
    int16_t  xMag;           // X magnetometer.
    int16_t  yMag;           // Y magnetometer.
    int16_t  zMag;           // Z magnetometer.
    uint16_t ias;            // Indicated air speed.
    int16_t windDir;        // Wind direction vector.
    uint16_t windMag;        // Wind magnitude.
    uint8_t  apMode;         // Autopilot mode.
    uint8_t  waypointType;   // K3 Waypoint Type
    uint8_t  waypointNum;    // K3 Waypoint Number
    uint16_t gpsDiscardCount; // Number of GPS packets discarded for CRC errors
    float  baro;            // Autopilot barometer values in meters (Added ICD Rev S)
    float  agl;             // Height Above launch in meters (Added ICD Rev S) 
    unsigned long long usTimeStamp; //OMAP time stamp of packet receipt
} fw_aircraft_metadata_t;



// Imager trigger packet definition. ------------------------------------------

typedef struct {
    uint8_t  imgSelect;      // Imager trigger selection bitfield.
    uint16_t trigID;         // Trigger ID.
    uint8_t  trigMode;       // Trigger mode.
    uint16_t trigPeriod;     // Trigger period (if continuous) in milliseconds.
} fw_imager_trigger_t;

// Imager zoom packet definition. --------------------------------------------

typedef struct {
    uint8_t imgSelect; // Imager  selection bitfield.
    uint8_t zoomMode;
    int8_t zoomRate;
    int8_t zoomSteps;  // Adjust zoom in discrete steps relative to current zoom position
} fw_imager_zoom_t;

typedef struct {
    uint8_t imgSelect; // Imager selection bitfield
    uint8_t videoStatus; // Set streaming video status
    uint32_t videoDstIP; // Set streaming video destination IPv4. Ignored when videoStatus is set to disabled.
    uint16_t videoDstPort; // Set streaming video destination port.  Ignored when videoStatus is set to disabled.    
    uint32_t cameraConfig; // Camera configuration 4k = byte 0/1 Camera 0 Position/Options, byte1/2 Camera 2 Position/Options) 
    uint8_t overlayConfig; // OSD overlay configuration
} fw_imager_preview_stream_setup_t;

typedef enum {
    VIDEO_STATUS_DISABLED = 0,
    VIDEO_STATUS_ENABLED = 1
} video_status_e;

typedef enum {
    DISABLED = 0,
    FULL = 1,
    PIP_LR = 2,
    TOP = 3,
    BOTTOM = 4,
    OVERLAY = 5
} video_position_config_e;

typedef enum {
    NORMAL = 0,
    NDVI_COLORMAP = 1
} video_options_config_e;

typedef enum {
    OVERLAY_DISABLED = 0,
    OVERLAY_ENABLED = 1
} overlay_config_e;

// Imager power packet definition. --------------------------------------------

typedef struct {
    uint8_t  imgSelect;      // Imager power selection bitfield.
    uint8_t  pwrMode;        // Power mode.
} fw_imager_power_t;


// Imager session packet definition. ------------------------------------------

typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint8_t  sessionName[128]; // Session name, a NULL terminated ASCII string. (Increased to 128 bits and fpf removed in Rev R)
    uint8_t  utcYear;        // UTC year - 1900.
    uint8_t  utcMonth;       // UTC month.
    uint8_t  utcDay;         // UTC day.
    uint8_t  utcHour;        // UTC hour.
    uint8_t  utcMinute;      // UTC minute.
    uint16_t utcMillisecond; // UTC millisecond.
    uint16_t sessionID;      // Session ID.
    uint16_t buildVersion;   // K3 Firmware Build Version
    uint8_t  aircraftType;   // Aircraft Type
    uint8_t  resumeSession;  // 1 = Resume session for same named session (Added in ICD Rev S)
} fw_imager_session_t;

typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint8_t  sessionName[128];// Session name, a NULL terminated ASCII string. (increased to 128 bytes in ICD Rev E)
    uint8_t  utcYear;        // UTC year - 1900.
    uint8_t  utcMonth;       // UTC month.
    uint8_t  utcDay;         // UTC day.
    uint8_t  utcHour;        // UTC hour.
    uint8_t  utcMinute;      // UTC minute.
    uint16_t utcMillisecond; // UTC millisecond.
    uint16_t sessionID;      // Session ID.
    uint16_t buildVersion;   // K3 Firmware Build Version
    uint8_t  aircraftType;   // Aircraft Type
    uint8_t  resumeSession; // Enable for adaptive scouting mission
} fw_imager_session_s_t;

typedef enum {
    IMAGER_SESSION_OPEN   = 0x00,
    IMAGER_SESSION_CLOSE  = 0x01,
    IMAGER_SESSION_KILL   = 0x02
} fw_imager_session_command_e;

// Video session packet definition. ------------------------------------------

typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint32_t exposureUs;     // Exposure Time us
    unsigned long long timeStamp;  // Microseconds since unix epoch
} fw_video_session_t;

// Video session advanced packet definition. ------------------------------------------
typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint32_t exposureUs;     // Exposure Time us
    unsigned long long timeStamp;  // Microseconds since unix epoch
    uint32_t bitrate;
    uint8_t gop;
    uint8_t metadataSource;
    uint8_t eStab;
    uint16_t aeTarget;
    uint16_t gain;
    uint16_t hResolution;
    uint16_t vResolution;
} fw_video_session_advanced_t;

// Video adjust packet definition. ------------------------------------------
typedef struct {
    uint32_t bitrate;
    uint8_t gop;
    uint8_t eStab;
    uint16_t aeTarget;
    uint16_t gain;
} fw_video_adjust_t;

// Exposure adjust packet definition. -----------------------------------------
typedef struct {
    uint32_t exposure_time_us;
    uint32_t analog_gain;
    uint32_t digital_gain;
} fw_exposure_adjust_t;

// Video Adjust Relative packet Definition. (Added ICD Rev S3) ------------------------------
typedef enum {
    NO_CHANGE       = 0x00,
    DECREASE        = 0x01,
    INCREASE        = 0x02,
    DECREASE_LOOP   = 0x03,
    INCREASE_LOOP   = 0x04
} exposure_relative_cmd_e ;


typedef struct {
    uint8_t imgSelect;  // Imager  selection bitfield.
    uint8_t evCommand;  // Relative command for exposure
    uint8_t isoCommand; // Relative command for iso
} fw_video_adjust_relative_t;

// Focus session packet definition. ------------------------------------------
typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint32_t exposureUs;     // Exposure Time us
} fw_focus_session_t;

// Still focus session packet definition. ------------------------------------------
typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint32_t exposureUs;     // Exposure Time us
} fw_still_focus_session_t;

// Imager trigger ack packet definition. --------------------------------------
typedef struct {
    uint8_t  imagerID;       // Imager ID, 0-7.
    uint16_t trigID;         // Trigger ID acknowledged.
} fw_imager_trigger_ack_t;


// Imager session ack packet definition. --------------------------------------
typedef struct {
    uint8_t  sessionCmd;     // Session command type.
    uint16_t sessionID;      // Session ID acknowledged.
    uint8_t imagerID;		   // Imager ID that responded to session open
} fw_imager_session_ack_t;

// Payload metadata packet definition. ----------------------------------------
typedef struct {
    uint8_t  imagerID;       // Imager ID, 0-7.
    uint8_t  imagerType;     // Imager type.
    uint16_t imagerVersion;  // Imager version.
    uint16_t imagerVFOV;     // Imager vertical field of view.
    uint16_t imagerHFOV;     // Imager horizontal field of view.
    uint16_t imagerZoom;     // Imager zoom.
    uint16_t memCapacity;    // Memory capacity.
    uint8_t  memUsed;        // Memory used, 0-100%.
    int16_t  mntRoll;        // Imager mount roll angle.
    int16_t  mntPitch;       // Imager mount pitch angle.
    int16_t  mntYaw;         // Imager mount yaw angle.
    uint8_t  pwrMode;        // Power mode.
    uint8_t  sessionStatus;  // Session status.
    uint32_t sessionImgCnt;  // Count of images in current session.
    uint8_t  captureStatus;  // Bit 1: Recording Video, Bit 2: Auto-triggering images (ICD Rev S)
    uint16_t minHFOV;        // Minimum imager HFOV (ICD Rev S)
    uint16_t minVFOV;        // Minimum imager VFOV (ICD Rev S)
    uint16_t maxHFOV;        // Maximum imager HFOV (ICD Rev S)
    uint16_t maxVFOV;        // Maximum imager VFOV (ICD Rev S)
    uint8_t  videoStatus;    // Streaming video status (0=disabled, 1=enabled) (ICD Rev S)
    uint32_t videoDstIP;     // Streaming video destination port (hex, 2 per octet, 4 octets) (ICD Rev S)
    uint16_t videoDstPort;   // Streaming video destination port (ICD Rev S)
} fw_payload_metadata_t;

// Elevation metadata packet definition. ------------------------------------------
typedef struct {
    uint8_t  mode;       // Which field to use
    uint8_t  dataValid;  // Which fields contain valid data
    float cameraAGL;   // Camera height above ground level
    float terrainMSL;  // Terrain height above MSL (Above EGM96 Geoid)
    float cameraMSL;   // Camera height above MSL (Above EGM96 Geoid)
} fw_elevation_metadata_t;

typedef enum {
    ELEVATION_MODE_CAMERA_AGL   = 0x00,
    ELEVATION_MODE_TERRAIN_MSL  = 0x01,
    ELEVATION_MODE_CAMERA_MSL   = 0x02,
} fw_elevation_mode_e;

typedef enum {
    ELEVATION_VALID_MASK_CAMERA_AGL   = 0x01,
    ELEVATION_VALID_MASK_TERRAIN_MSL  = 0x02,
    ELEVATION_VALID_MASK_CAMERA_MSL   = 0x04,
} fw_elevation_data_valid_mask_e;


//  Imager Data Ready packet definition. (Added ICD Rev S) --------------------------

typedef struct {
    uint8_t  imagerID;       // Imager ID, 0-7.
    uint8_t  fileName[48];   // The name and path from the session folder of the new file
} fw_imager_data_ready_t;

// System Time packet Definition. (Added ICD Rev S2) ---------------------------------

typedef struct {
    uint8_t  command;      				// Get or Set time 
    uint16_t requestID;    				// Unique ID for the request 
    uint64_t timeStamp;     	        // us Since epoch to set
} fw_system_time_t;

// System time commands
typedef enum {
	SYSTIME_GET = 0x00,
	SYSTIME_SET = 0x01,
} sys_time_cmd_e;

// System Time Ack packet Definition. (Added ICD Rev S2) ------------------------------

typedef struct {
    uint16_t requestID;    	    // Unique ID for the request 
    uint64_t timeStamp;     	// us Since epoch of processor
    uint64_t  bootTime;     	// ns since processor boot
} fw_system_time_ack_t;

#endif /* PACKETS_H_ */
