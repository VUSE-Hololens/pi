#pragma once

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <chrono>

// user includes
#include "Bufferizer.h" // bufferizes packets of data
#include "DataPacketizer.h" // makes packets of data
#include "Sensor.h" // parent class
#include "packets.h" // packet structures
#include "HTTPDownloader.h" // download HTTP packets
#include <turbojpeg.h> // turbojpeg for decompression
#include "DataProcessor.h" // for creating NDVI band data

//#include "transmit.h" // for cascading transmit
#include "compress.h"


class SenteraDouble4k : public Sensor // implements sensor
{
	// Pi -> Hololens NDVI jpg transmission control
	enum TransmitMode { fullFile, fileName }; // full file in single UDP packet or file name for download
	const char *MODE_NAMES[2] = { "Full File", "File Name" };

	// Variables
public:
	static const TransmitMode MODE = fullFile;
private:
	// class variables
	static const int BUFLEN = 512;											// Buffer length
	static const int IMG_FILENAME_LEN = 48;
	static const int IMG_FILENAME_DIR_LEN = 4;								// length of leading directory in img filepath
	int jpg_quality = 75;														// jpg compression quality

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
	const static int cams = 2;												// Double camera
	fw_imager_data_ready_t recent_images[cams];								// Store individual history of the last num_cameras images - no circular buffer

	fw_payload_metadata_t camera_metadata[cams];							// Store up to num_cameras worth of camera info
	bool camera_metadata_valid[cams];										// Indicates whether each ID was valid 
	unsigned long long camera_metadata_last_update_us[cams];				// Timestamp of last update 
	fw_system_time_ack_t recent_time_ack;									// The most recent system time acknowledgement data
	int imgReadyID;															// ID of most recent image ready
	HTTPDownloader http_downloader;											// downloader
	tjhandle _jpegDecompressor;

	// other
	uint8_t trigger_mask = 0x03;											// Default Trigger Mask
	int serv_status = -1;
	bool live_session = false;
	const static int timeout = 15000;										// milliseconds until timeout

	// cascading transmission
	//transmit transmitter;													// for transmitting cascade style
	compress compressor;													// for testing compression

// methods
private:
	//sending and receiving data to/from sentera
	int configure_socket(int myport, sockaddr_in& si_other, bool bind_socket);
	int configure_receive(int myport, sockaddr_in& si_other);
	int startServer();

	// initializing session
	int makeImagerTriggerPacket(uint8_t mode, uint32_t period, uint8_t *buf);
	int makeStillCapturePacket(uint8_t option, const char *sessionName, uint8_t *buf);

	// listener to receive and process data
	int sessionListener();

	//receiving data
	int query_status_packet();

	//processing data
	int processImage(int cam);
	std::string makeUrlPath(uint8_t *filename);

	// for transmitting cascade style
	void sendNDVI(int quality);


public:
	SenteraDouble4k();
	SenteraDouble4k(Transform offset);
	~SenteraDouble4k();
	int Start();
	int Stop();
	std::vector<Frame> Data();
	int updateTrigger();
};