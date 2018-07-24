#pragma once

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// user includes
#include "Bufferizer.h"
#include "DataPacketizer.h"
#include "Sensor.h" // extends
#include "packets.h"

// defines
#define BUFLEN 512

// PACKET HEADERS - TO PAYLOAD
#define SEND_AIRCRAFT_MEDATADA 0x01 // To Payload Aircraft Metadata
#define SEND_IMAGER_TRIGGER 0x02 // To Payload Imager Trigger
#define SEND_IMAGER_POWER 0x03 // To Payload Imager Power
#define SEND_STILL_CAPTURE 0x04 // To Payload Still Capture Session
#define SEND_VIDEO_CAPTURE 0x05 // To Payload Video Session
#define SEND_VIDEO_FOCUS 0x06 // To Payload Video Focus Session
#define SEND_STILL_FOCUS 0x07 // To Payload Still Focus Session
#define SEND_AUTOPILOT_PING_RESPONSE 0x08 // To Payload Autopilot Ping Response
#define SEND_VIDEO_SESSION_ADVANCED 0x09 // To Payload Video Session Advanced
#define SEND_VIDEO_ADJUST 0x0A // To Payload Video Adjust
#define SEND_PAYLOAD_RESERVED 0x0B // To Payload(Reserved)
#define SEND_IMAGER_ZOOM 0x0C // To Payload Imager Zoom
#define SEND_IMAGER_PREVIEW_STREAM_SETUP 0x0D // To Payload Imager Preview Stream Setup
#define SEND_ELEVATION_METADATA 0x0E // To Payload Elevation Metadata
#define SEND_SYSTEM_TIME 0x0F // To Payload System Time
#define SEND_VIDEO_ADJUST_RELATIVE 0x10 // To Payload Video Adjust Relative
#define SEND_EXPOSURE_ADJUST 0x11 // To Payload Exposure Adjust

// PACKET HEADERS - FROM PAYLOAD
#define RECV_PAYLOAD_METADATA 0x81 // From Payload Payload Metadata
#define RECV_IMAGER_TRIGGER_ACK 0x82 // From Payload Imager Trigger Ack
#define RECV_STILL_CAPTURE_SESSION_ACK 0x83 // From Payload Still Capture Session Ack
#define RECV_AUTOPILOT_PING_REQUEST 0x84 // From Payload Autopilot Ping Request
#define RECV_IMAGE_DATA_READY 0x85 // From Payload Image Data Ready
#define RECV_SYSTEM_TIME_ACK 0x8F // From Payload System Time Ack
#define RECV_FOCUS_SCORE 0xD0 // From Payload Focus Score
#define RECV_PAYLOAD_EXCEPTION 0xFF // From Payload Payload Exception

class SenteraDouble4k : public Sensor // implements sensor
{
// Variables
public:

private:
	// class variables
	struct sockaddr_in si_other_send;										// Socket address of camera
	struct sockaddr_in si_other_rec;										// Socket address receiving
	int slen_send = sizeof(si_other_send);
	int slen_rec = sizeof(si_other_rec);

	int s_send, s_rec;														// Sending and Receiving Sockets

	const std::string server_ipaddr = "192.168.143.141";					// Default IP of camera - was: char server_ipaddr[80]
	const std::string local_ipaddr = PI_IP;									// Default local IP
	uint16_t cameraPort = 60530;											// Default port of camera
	uint16_t localPort = 60531;												// Default local port for receiving

	// for receiving
	const static int num_cameras = 2;										// Double camera
	//const int FILE_HISTORY_SIZE = 2;										// The number of saved files to store
	//fw_imager_data_ready_t recent_images[num_cameras][FILE_HISTORY_SIZE];	// Store individual history of the last num_cameras images recorded in a circular buffer of FILE_HISTORY_SIZE
	//int recent_images_length[num_cameras];								// The number of recent images stored in the buffer
	//int recent_images_start[num_cameras];									// The current index of the circular buffer
	fw_imager_data_ready_t recent_images[num_cameras];						// Store individual history of the last num_cameras images - no circular buffer

	fw_payload_metadata_t camera_metadata[num_cameras];						// Store up to num_cameras worth of camera info
	bool camera_metadata_valid[num_cameras];								// Indicates whether each ID was valid 
	unsigned long long camera_metadata_last_update_us[num_cameras];			// Timestamp of last update 
	fw_system_time_ack_t recent_time_ack;									// The most recent system time acknowledgement data

	uint8_t trigger_mask = 0x03;											// Default Trigger Mask
	int serv_status = -1;
	bool live_session = false;

// methods
private:
	int configure_socket(int myport, sockaddr_in& si_other, bool bind_socket);
	int configure_receive(int myport, sockaddr_in& si_other);
	int makeSessionPacket(uint8_t sessionType, uint8_t *buf);
	int query_status_packet();
	int startServer();
	int initializeSession(uint8_t sessionType);
	int retrieveCurrentData();
	std::string makeFilePath(uint8_t *filename);

public:
	SenteraDouble4k(Transform offset);
	~SenteraDouble4k();
	void Start();
	void Stop();
	Frame Data();
};