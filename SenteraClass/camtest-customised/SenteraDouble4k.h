#pragma once

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <thread>

// user includes
#include "Bufferizer.h"
#include "DataPacketizer.h"
#include "Sensor.h" // extends
#include "packets.h"

// defines
#define BUFLEN 512

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
	fw_imager_data_ready_t recent_images[num_cameras];						// Store individual history of the last num_cameras images - no circular buffer

	fw_payload_metadata_t camera_metadata[num_cameras];						// Store up to num_cameras worth of camera info
	bool camera_metadata_valid[num_cameras];								// Indicates whether each ID was valid 
	unsigned long long camera_metadata_last_update_us[num_cameras];			// Timestamp of last update 
	fw_system_time_ack_t recent_time_ack;									// The most recent system time acknowledgement data

	std::string currentSessionName;
	uint8_t trigger_mask = 0x03;											// Default Trigger Mask
	int serv_status = -1;
	bool live_session = false;

// methods
private:
	//sending and receiving data to/from sentera
	int configure_socket(int myport, sockaddr_in& si_other, bool bind_socket);
	int configure_receive(int myport, sockaddr_in& si_other);
	int startServer();
	// initializing session
	int makeImagerTriggerPacket(uint8_t mode, uint32_t period, uint8_t *buf);
	int makeStillCapturePacket(uint8_t option, std::string sessionName, uint8_t *buf);

	//retreiving data
	int query_status_packet();
	int retrieveCurrentData();
	std::string makeFilePath(uint8_t *filename, bool url);

public:
	SenteraDouble4k(Transform offset);
	~SenteraDouble4k();
	int Start();
	int Stop();
	Frame Data();
};