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

public class SenteraDouble4k // imolements sensor
{
// Variables
public:

private:
	const int num_cameras;
	const int FILE_HISTORY_SIZE;

// methods
public:
	SenteraDouble4k();
	~SenteraDouble4k();
	int startServer(byte sessionType);
	int initialize_session(uint8_t sessionType);

private:
	char[] makeStillCapturePacket(char sessionCmd);
	int configure_socket(int myport, sockaddr_in& si_other, bool bind_socket);
	int configure_receive(int myport, sockaddr_in& si_other);
	void makeSessionPacket(uint8_t sessionType, uint8_t *buf);
	int query_status_packet();

};