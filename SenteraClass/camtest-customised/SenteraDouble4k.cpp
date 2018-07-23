
// includes
#include "SenteraDouble4k.h"
#include <string>

// class variables
struct sockaddr_in si_other_send;										// Socket address of camera
struct sockaddr_in si_other_rec;										// Socket address receiving
int slen_rec = sizeof(si_other_rec);
int slen_send = sizeof(si_other_send);

int s_send, s_rec;														// Sending and Receiving Sockets

std::string server_ipaddr("192.168.143.141");							// Default IP of camera - was: char server_ipaddr[80]
std::string local_ipaddr(Sensor::PI_IP);								// Default local IP
uint16_t cameraPort = 60530;											// Default port of camera
uint16_t localPort = 60531;												// Default local port for receiving

// for receiving
const int num_cameras = 2;												// Double camera
const int FILE_HISTORY_SIZE = 2;										// The number of saved files to store
fw_imager_data_ready_t recent_images[num_cameras][FILE_HISTORY_SIZE];	// Store individual history of the last num_cameras images recorded in a circular buffer of FILE_HISTORY_SIZE
int recent_images_length[num_cameras];									// The number of recent images stored in the buffer
int recent_images_start[num_cameras];									// The current index of the circular buffer

fw_payload_metadata_t camera_metadata[num_cameras];						// Store up to num_cameras worth of camera info
bool camera_metadata_valid[num_cameras];								// Indicates whether each ID was valid 
unsigned long long camera_metadata_last_update_us[num_cameras];			// Timestamp of last update 
fw_system_time_ack_t recent_time_ack;									// The most recent system time acknowledgement data

uint8_t trigger_mask = 0x03;											// Default Trigger Mask
int serv_status = -1;
bool live_session = false;

Frame imgData;															// Frame that holds most recent image data

SenteraDouble4k::SenteraDouble4k()
{
	struct timeval currTime;

	fw_payload_metadata_t status;
	// Assume we start without a connection
	for (int i = 0; i<num_cameras; i++) {
		camera_metadata_valid[i] = false;
		camera_metadata_last_update_us[i] = 0;
	}

	// Reset our circular buffer
	for (int i = 0; i< num_cameras; i++) {
		recent_images_length[i] = 0;
		recent_images_start[i] = 0;
	}

	// configure send and receive sockets
	serv_status = startServer();
	if (serv_status) {
		printf("Made connection with camera!\n"); //DEBUG
	}

}

SenteraDouble4k::~SenteraDouble4k()
{
	close(s_send);
	close(s_rec);
}

void SenteraDouble4k::Start() {
	initializeSession(SEND_IMAGER_TRIGGER);
}

// starts server by setting up send and receive sockets
int SenteraDouble4k::startServer() {

	if (serv_status > -1) {
		printf("Server already running!\n"); //DEBUG
		return 0;
	}

	// If we run locally on the camera, don't bind to the port or we fail
	bool bind_send_socket = true;
	if (local_ipaddr.compare(server_ipaddr) == 0)
	{
		fprintf(stderr, "!! Local camera running, skipping bind to %d !!", cameraPort); //DEBUG
		bind_send_socket = false;
	}

	// Configure Sending Socket
	// If we try to bind sending socket, and it is multicast, the program breaks
	s_send = configure_socket(cameraPort, si_other_send, bind_send_socket);
	if (!s_send)
	{
		fprintf(stderr, "!! Unable to configure sending socket. !!"); //DEBUG
		return -1;
	}

	// Configure Receiving Socket
	s_rec = configure_receive(localPort, si_other_rec);
	if (!s_rec)
	{
		fprintf(stderr, "!! Unable to configure receiving socket. !!"); //DEBUG
		return -1;
	}
	return 1;
}

int SenteraDouble4k::initializeSession(uint8_t sessionType)
{
	// check if server is set up
	if (serv_status == -1) {
		printf("Server not initialized! Cannot start session.");
		return -1;
	}

	// initialize packet to fill and send
	uint8_t buf[BUFLEN];

	// make packet of data 
	int packet_length = makeSessionPacket(sessionType, buf);
	if (packet_length <= 0)
	{
		printf("Failed to create packet");
		return -1;
	}

	//DEBUG
	printf("Packet Created. Length: %d, sample: %X\n", packet_length, buf[2]);

	//send packet of data
	if (packet_length > 0 && sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1)
	{
		printf("Failed to send packet: %d", errno); //DEBUG
		return -1;
	}

	// listen for updates
	live_session = true;
	while (live_session)
	{
		bool received_data = false;
		while (!received_data)
		{
			received_data = (query_status_packet() == 1);
		}
	}
	return 0;
}

void SenteraDouble4k::Stop() {
	live_session = false;
	close(s_send);
	close(s_rec);
	return;
}

// Configures a given socket returning a socket ID, -1 indicates failure
int SenteraDouble4k::configure_socket(int myport, sockaddr_in& si_other, bool bind_socket)
{
	struct in_addr local_interface;
	int i, s;
	int broadcast = 1;
	int opt = 1;

	// Open network connections
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		fprintf(stderr, "!! Failed to create socket !!\n");
		return -1;
	}

	/* Dont need to be sending broadcast data 
	// Configure socket for sending broadcast data 
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
	{
		fprintf(stderr, "!! Failed allow broadcasts. !!\n");
		return -1;
	}*/ 

	// Allow binding to address already in use
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		fprintf(stderr, "!! Failed allow reuse address. !!\n");
		return -1;
	}

	// Refer IP address and port to the socket
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(myport);
	if (inet_aton(server_ipaddr.c_str(), &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "!! inet_aton() failed !!\n");
		return -1;
	}

	// An odd case occurs when you try to bind to a port that already contains a bind.  
	// I'm not sure exactly what is happened, but somehow the socket reference, s becomes
	// invalid, even without the return code explicitely marking a failure.  I'd love to
	// sort this out later, but for now we simply skip it when binding on the send port 
	// I don't think the sender has to bind?
	if (bind_socket)
	{
		struct sockaddr_in bind_addr;
		memset(&bind_addr, 0, sizeof(struct sockaddr_in));
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_port = htons(myport); // locally bound port
		if ((bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) < 0)
		{
			fprintf(stderr, "bind error with port\n");
			return -1;
		}
	}

	return s;
}

// UNIX socket-friendly function for configuring the receiving socket
int SenteraDouble4k::configure_receive(int myport, sockaddr_in& si_other)
{
	struct in_addr local_interface;
	int i, s;
	int broadcast = 1;
	int opt = 1;

	// Open network connections
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		fprintf(stderr, "!! Failed to create socket !!\n");
		return -1;
	}

	// Configure socket for sending broadcast data 
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
	{
		fprintf(stderr, "!! Failed allow broadcasts. !!\n");
		return -1;
	}

	// Allow binding to address already in use
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		fprintf(stderr, "!! Failed allow reuse address. !!\n");
		return -1;
	}

	// Refer IP address and port to the socket
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(myport);
	if (inet_aton(server_ipaddr.c_str(), &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "!! inet_aton() failed !!\n");
		return -1;
	}

	// Bind to the multicast address now
	struct sockaddr_in bind_addr;
	memset(&bind_addr, 0, sizeof(struct sockaddr_in));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(myport); // locally bound port
	if ((bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) < 0)
	{
		fprintf(stderr, "bind error with receive port\n");
		return -1;
	}

	// Attempt to join the multicast group for the camera
	// If we specified a multicast IP for the camera, this should succeed
	// If not, fallback to unicast binding

	// An odd case occurs when you try to bind to a port that already contains a bind.
	// I'm not sure exactly what is happened, but somehow the socket reference, s becomes
	// invalid, even without the return code explicitely marking a failure.  I'd love to
	// sort this out later, but for now we simply skip it when binding on the send port
	// Receiver has to bind

	return s;
}

int SenteraDouble4k::makeSessionPacket(uint8_t sessionType, uint8_t *buf)
{
	int packet_length = -1;
	switch (sessionType & 0xFF) {
	case SEND_IMAGER_TRIGGER: {
		fw_imager_trigger_t imager_trigger = DataPacketizer::trigger(trigger_mask); // Construct new session packet
		packet_length = Bufferizer::trigger(imager_trigger, buf); // Load the new packet into the buffer
		break;
	}
	case SEND_STILL_CAPTURE: {
		fw_imager_session_t imager_session = DataPacketizer::session();
		packet_length = Bufferizer::session(imager_session, buf);
		break;
	}
	case SEND_VIDEO_CAPTURE: {
		fw_video_session_t video_session = DataPacketizer::video();
		packet_length = Bufferizer::video(video_session, buf);
		break;
	}
	case SEND_STILL_FOCUS: {
		fw_still_focus_session_t still_focus_session = DataPacketizer::sf();
		packet_length = Bufferizer::sf(still_focus_session, buf);
		break;
	}
	case SEND_VIDEO_ADJUST: {
		fw_video_adjust_t video_adjust = DataPacketizer::video_adjust();
		packet_length = Bufferizer::videoadjust(video_adjust, buf);
		break;
	}
	case SEND_IMAGER_ZOOM: {
		fw_imager_zoom_t imager_zoom = DataPacketizer::zoom(trigger_mask);
		packet_length = Bufferizer::zoom(imager_zoom, buf);
		break;
	}
	case SEND_SYSTEM_TIME: {
		printf("SystemTime option not implemented yet.");
		//fw_system_time_t system_time = DataPacketizer::system_time();
		//packet_length = Bufferizer::system_time(system_time, buf);
		break;
	}
	case SEND_EXPOSURE_ADJUST: {
		fw_exposure_adjust_t exposure_adjust = DataPacketizer::exposureadjust();
		packet_length = Bufferizer::exposureadjust(exposure_adjust, buf);
		break;
	}
	default: {
		printf("Cannot initialize session of type ");
		return -1;
	}
	}
	return packet_length;

}

int SenteraDouble4k::query_status_packet()
{
	uint8_t rec_buf[BUFLEN];
	int rec_data;
	int current_packet = 0;
	errno = 0;
	int newdata_received = 0;

	// Keep reading and processing until the packets are emtpy.
	// Since each packet can be from a different imagerID, we process each one
	// old buffers will override new ones.
	// Non-blocking receives with no data will simply not write to the buffer (tested)
	do
	{
		// Check for receiving packet; recvfrom returns length of packet
		current_packet = recvfrom(s_rec, rec_buf, BUFLEN, MSG_DONTWAIT, (struct sockaddr *) &si_other_rec, (socklen_t*)&slen_rec);
		if (current_packet <= 0)
		{
			// Would block and eagain just say there is no data
			if (!(errno == EWOULDBLOCK || errno == EAGAIN)) {
				perror("Error receiving data packet.");
				return -1;
			}
		}

		// Minimum size for both status and new data events
		// As well as FW Header check
		else if (current_packet < 24 || !(rec_buf[0] == 0x46 && rec_buf[1] == 0x57))
		{
			printf("Recv Packet Header Failure");
			return 0;
		}
		else if (rec_buf[2] == RECV_PAYLOAD_METADATA) 
		{
			//printf("Received Payload Metadata"); //DEBUG

			// Make sure our packet length is long enough
			if (!(rec_buf[3] >= 0x19 && rec_buf[4] == 0x00)) return 0;

			// Good packet! Go ahead and process.
			int n = 5;
			fw_payload_metadata_t status;
			status.imagerID = rec_buf[n++];
			status.imagerType = rec_buf[n++];
			status.imagerVersion = rec_buf[n++];
			status.imagerVersion += rec_buf[n++] << 8;
			status.imagerVFOV = rec_buf[n++];
			status.imagerVFOV += rec_buf[n++] << 8;
			status.imagerHFOV = rec_buf[n++];
			status.imagerHFOV += rec_buf[n++] << 8;
			status.imagerZoom = rec_buf[n++];
			status.imagerZoom += rec_buf[n++] << 8;
			status.memCapacity = rec_buf[n++];
			status.memCapacity += rec_buf[n++] << 8;
			status.memUsed = rec_buf[n++];
			status.mntRoll = rec_buf[n++];
			status.mntRoll += rec_buf[n++] << 8;
			status.mntPitch = rec_buf[n++];
			status.mntPitch += rec_buf[n++] << 8;
			status.mntYaw = rec_buf[n++];
			status.mntYaw += rec_buf[n++] << 8;
			status.pwrMode = rec_buf[n++];
			status.sessionStatus = rec_buf[n++];
			status.sessionImgCnt = rec_buf[n++];
			status.sessionImgCnt += rec_buf[n++] << 8;
			status.sessionImgCnt += rec_buf[n++] << 16;
			status.sessionImgCnt += rec_buf[n++] << 24;

			// Rev S Updates
			// Rev S = 41 bytes + 5 header + crc
			if (current_packet >= 47) {
				status.captureStatus = rec_buf[n++];
				status.minHFOV = rec_buf[n++];
				status.minHFOV += rec_buf[n++] << 8;
				status.minVFOV = rec_buf[n++];
				status.minVFOV += rec_buf[n++] << 8;
				status.maxHFOV = rec_buf[n++];
				status.maxHFOV += rec_buf[n++] << 8;
				status.maxVFOV = rec_buf[n++];
				status.maxVFOV += rec_buf[n++] << 8;
				status.videoStatus = rec_buf[n++];
				status.videoDstIP = rec_buf[n++];
				status.videoDstIP += rec_buf[n++] << 8;
				status.videoDstIP += rec_buf[n++] << 16;
				status.videoDstIP += rec_buf[n++] << 24;
				status.videoDstPort = rec_buf[n++];
				status.videoDstPort += rec_buf[n++] << 8;
			}
			else
			{
				status.captureStatus = 0;
				status.minHFOV = 0;
				status.minVFOV = 0;
				status.maxHFOV = 0;
				status.maxVFOV = 0;
				status.videoStatus = 0;
				status.videoDstIP = 0;
				status.videoDstPort = 0;
			}

			// Get the time we received the packet (approx)
			struct timeval currTime;
			gettimeofday(&currTime, NULL);
			unsigned long long timestamp = (((unsigned long long)currTime.tv_sec) * 1000000) + ((unsigned long long)currTime.tv_usec);

			// Store the most recent packet for each camera
			// A packet can be for more than one camera.
			for (int i = 0; i < num_cameras; i++)
			{
				if (status.imagerID & (0x01 << i))
				{
					camera_metadata[i] = status;
					camera_metadata_valid[i] = true;
					camera_metadata_last_update_us[i] = timestamp;
				}
			}

			newdata_received = 1;
		}
		// Handle new image avilable packets
		else if (rec_buf[2] == RECV_IMAGE_DATA_READY)
		{
			// Make sure our packet length is long enough
			if (!(rec_buf[3] >= 0x21 && rec_buf[4] == 0x00)) return 0;

			// Good packet! Go ahead and process.
			int n = 5;
			fw_imager_data_ready_t new_image;
			new_image.imagerID = rec_buf[n++];

			for (int i = 0; i < 48; ++i)
			{
				new_image.fileName[i] = rec_buf[n];
				n++;
			}

			// Store the packet in the appropriate location of the circular buffer
			for (int i = 0; i < num_cameras; i++)
			{
				if (new_image.imagerID & (0x01 << i))
				{
					// Handle circular buffer wraparound
					recent_images_length[i]++;
					if (recent_images_length[i] > FILE_HISTORY_SIZE)
					{
						recent_images_length[i] = FILE_HISTORY_SIZE;
						recent_images_start[i]++;
						recent_images_start[i] = recent_images_start[i] % FILE_HISTORY_SIZE;
					}


					int cur_idx = (recent_images_start[i] + recent_images_length[i] - 1) % FILE_HISTORY_SIZE;
					recent_images[i][cur_idx] = new_image;
				}
			}

			newdata_received = 1;

		}
		// Handle Time Ack Packets
		else if (rec_buf[2] == RECV_SYSTEM_TIME_ACK)
		{
			printf("Receive System Time Ack"); //DEBUG

			// Make sure our packet length is long enough
			if (!(rec_buf[3] >= 0x12 && rec_buf[4] == 0x00)) return 0;

			// Good time ack packet, temporarily store the response
			int n = 5;
			recent_time_ack.requestID = rec_buf[n++] << 0;
			recent_time_ack.requestID += rec_buf[n++] << 8;
			recent_time_ack.timeStamp = (unsigned long long) rec_buf[n++] << 0;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 8;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 16;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 24;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 32;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 40;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 48;
			recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 56;
			recent_time_ack.bootTime = (unsigned long long) rec_buf[n++] << 0;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 8;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 16;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 24;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 32;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 40;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 48;
			recent_time_ack.bootTime += (unsigned long long) rec_buf[n++] << 56;
		}
		else if (rec_buf[2] = RECV_IMAGER_TRIGGER_ACK)
		{
			printf("Received Imager Trigger Acknowledgement\n");
		}
		else
		{
			printf("INCOMPLETE PACKET of Length: %d", current_packet);
		}

	} while (current_packet > 0);

	return newdata_received;
}

int retrieveCurrentData() {
	
	//std::string rgbStr = makeUrlStr(recent_images[0][recent_images_start[0]].fileName);
	//std::string nirStr = makeUrlStr(recent_images[1][recent_images_start[1]].fileName);
	//printf((const char*)rgbStr + "\n");
	//printf((const char*)nirStr + "\n");

	//http_client client(rgbStr);
	//http_response response;
	//response = client.request(methods::GET).get(); // unsure of arguments to this request method
	

}

Frame SenteraDouble4k::Data() {
	return *data;
}

/*
std::string makeUrlString(uint8_t *filename) {
	//http://192.168.143.141:8080/cur_session?path=/RGB/IMG_000001.jpg
	std::string urlStr("http://");
	urlStr += server_ipaddr;
	urlStr += ":";
	urlStr += "8080";
	urlStr += "/cur_session?path=/";
	urlStr += std::string((const char*)filename);
	return urlStr;
}*/

