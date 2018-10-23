// Header for Sentera Camera Class
// Henry Bristol
// July 25, 2018

// includes
#include "SenteraDouble4k.h"
#include <fstream>

SenteraDouble4k::SenteraDouble4k() : SenteraDouble4k::SenteraDouble4k(offset){}

// constructor
// initializes compressor, runs startServer: creates, starts send and receive ethernet sockets
// run first by main
SenteraDouble4k::SenteraDouble4k(Transform _offset) : Sensor(_offset, this->cams)
{
	// Assume we start without a connection
	for (int i = 0; i<num_cameras; i++) {
		camera_metadata_valid[i] = false;
		camera_metadata_last_update_us[i] = 0;
	}

	// setup compressor/decompressor
	_jpegDecompressor = tjInitDecompress();

	// configure send and receive sockets
	serv_status = startServer();
	if (serv_status) {
		fprintf(stderr, "Successfully Setup Sockets!\n"); //DEBUG
	}
}

SenteraDouble4k::~SenteraDouble4k()
{
	close(s_send);
	close(s_rec);
	tjDestroy(_jpegDecompressor);
}

// run second by main
int SenteraDouble4k::Start() {
	const char currentSessionName[128] = "TestingName1";

	// ensures servers started successfuly by constructor
	if (serv_status == -1) {
		fprintf(stderr, "Server not initialized! Cannot start session.");
		return -1;
	}
	
	// initialize packet to fill and send
	uint8_t buf[BUFLEN];

	// make packet of still capture session data 
	int packet_length = makeStillCapturePacket((uint8_t)0, currentSessionName, buf);
	if (packet_length <= 0) {
		fprintf(stderr, "Failed to create packet");
		return -1;
	}

	// DEBUG
	//fprintf(stderr, "Still Capture Packet Created. Length: %d, Type: %X, Session name: %s\n", 
		//packet_length, buf[2], currentSessionName);

	// send packet of still capture session data
	if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1) {
		fprintf(stderr, "Failed to send packet: %d", errno); //DEBUG
		return -1;
	}
	//DEBUG printf("Sent still capture packet");
	fprintf(stderr, "Started and listening for data! Session Name: %s\n", currentSessionName);
	
	// infinite listening and processing loop
	return sessionListener();

	//return 0;
}

int SenteraDouble4k::sessionListener() {
	// listen for updates
	live_session = true;
	int recvType = 0;
	bool firstLoop = true;

	while (live_session)
	{
		// havent yet received data
		bool received_data = false;

		// create timer
		auto starttime = std::chrono::system_clock::now();
		auto endtime = std::chrono::system_clock::now();

		while (!received_data)
		{
			// debug
			//std::cout << "Checking if sentera sent data packet...\n";
			
			// query for new data
			recvType = query_status_packet();

			// received a new packet?
			received_data = (recvType >= 1);

			endtime = std::chrono::system_clock::now();

			// if no new data received
			if (!received_data) {
				// check if we've timed out, and send packet to stop session if so.
				if (std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime).count() > timeout) {
					fprintf(stderr, "System Timeout: No UDP packets received in %d milliseconds.\n", timeout);
					Stop();
					return -1;
				}
				// else if we havent timed out, query for a new packet
				continue;
			}

			// if new data is ready to process, do that
			if (recvType == fw_packet_type_e::IMAGER_DATA_READY) {
				if (firstLoop) {
					// debug
					fprintf(stderr, "Received first data from camera, processing...\n");
					firstLoop = false;
				}

				//DEBUG printf("Images ready for Camera %d\n", imgReadyID);

				// prepares images
				processImage(imgReadyID); // process data for appropriate image

				//DEBUG printf("Images processed for Camera %d\n", imgReadyID);
				//DEBUG printf("Bands Filtered for Camera %d\n", imgReadyID);

				// send NDVI image if ready
				if (imgReadyID == 2 && getUpdated()) sendNDVI(jpg_quality); 
				//DEBUG printf("NDVI data sent \n");
			}
		}
		// when data is received, reset timer
		starttime = endtime;
	}
	return 0;
}

int SenteraDouble4k::Stop() {
	// make packet of still capture session data to stop session
	uint8_t buf[BUFLEN];
	int packet_length = makeStillCapturePacket((uint8_t)1, "Stop", buf); // 1 to close session
	if (packet_length <= 0) {
		fprintf(stderr, "Failed to create packet");
		return -1;
	}

	// send packet of still capture session data
	if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1) {
		fprintf(stderr, "Failed to send packet: %d", errno); //DEBUG
		return -1;
	}

	live_session = false;
	return 0;
}

int SenteraDouble4k::updateTrigger() {
	//check server
	if (serv_status == -1) {
		fprintf(stderr, "Server not initialized! Cannot start session.");
		return -1;
	}

	// initialize packet to fill and send
	uint8_t buf[BUFLEN];

	// make packet of trigger data 
	int packet_length = makeImagerTriggerPacket((uint8_t)2, (uint32_t)1000, buf);
	if (packet_length <= 0) {
		fprintf(stderr, "Failed to create packet");
		return -1;
	}
	//printf("Trigger Packet Created. Length: %d, Type: %X\n", packet_length, buf[2]); 	//DEBUG

	//send packet of trigger data
	if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1) {
		fprintf(stderr, "Failed to send packet: %d", errno); //DEBUG
		return -1;
		return -1;
	}
}

// starts server by setting up send and receive sockets
int SenteraDouble4k::startServer() {
	// serv_status set to -1 in .h, returned as 1 by this method
	if (serv_status > -1) {
		fprintf(stderr, "Server already running!\n"); //DEBUG
		return 0;
	}

	// If we run locally on the camera, don't bind to the port or we fail
	bool bind_send_socket = true;
	// if Pi IP and serve IP (senter IP) are equal, print error and don't bind
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

// Configures a given socket returning a socket ID, -1 indicates failure
	// myPort: sentera's port
	// si_other: sentera destination, updated by reference and used later
	// bind_socket: bind created socket to specified address?
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
	// myport: local pi ethernet port
	// si_other: destination address of local pi's ethernet receiving socket, updated by reference and used later
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

int SenteraDouble4k::makeStillCapturePacket(uint8_t option, const char *sessionName, uint8_t *buf) { // 0x00 to Open, 0x01 to close
	fw_imager_session_t imager_session = DataPacketizer::session(option, sessionName); 
	return Bufferizer::session(imager_session, buf);
}

int SenteraDouble4k::makeImagerTriggerPacket(uint8_t mode, uint32_t period, uint8_t *buf) {
	fw_imager_trigger_t imager_trigger = DataPacketizer::trigger(trigger_mask, mode, period); // Construct new session packet
	return Bufferizer::trigger(imager_trigger, buf); // Load the new packet into the buffer
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

	do //DEBUG - test do while loop necessity
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
			fprintf(stderr, "Recv Packet Header Failure");
			return 0;
		}
		else if (rec_buf[2] == fw_packet_type_e::PAYLOAD_METADATA)
		{
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

			// 

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
					// TODO: check sensor band data stuff/ptr stuff
					sensor_data[i].FOVx = status.imagerHFOV / 100.0;
					sensor_data[i].FOVy = status.imagerVFOV / 100.0;
					camera_metadata[i] = status;
					camera_metadata_valid[i] = true;
					camera_metadata_last_update_us[i] = timestamp;
				}
			}

			newdata_received = fw_packet_type_e::PAYLOAD_METADATA;
		}
		// Handle new image avilable packets
		else if (rec_buf[2] == fw_packet_type_e::IMAGER_DATA_READY)
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

			if (new_image.imagerID == (uint8_t)1) {
				recent_images[0] = new_image;
				imgReadyID = 1;
			}
			else if (new_image.imagerID == (uint8_t)2) {
				recent_images[1] = new_image;
				imgReadyID = 2;
			}
			else {
				imgReadyID = 0;
				fprintf(stderr, "Imager ID not 1 or 2: Failed to store new image data\n");
				return -1;
			}

			newdata_received = fw_packet_type_e::IMAGER_DATA_READY;

		}
		// Handle Time Ack Packets
		else if (rec_buf[2] == fw_packet_type_e::SYSTEM_TIME_ACK)
		{
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

			newdata_received = fw_packet_type_e::SYSTEM_TIME_ACK;
		}
		else if (rec_buf[2] = fw_packet_type_e::IMAGER_TRIGGER_ACK)
		{
			fprintf(stderr, "Received Imager Trigger Acknowledgement\n");
			newdata_received = fw_packet_type_e::IMAGER_TRIGGER_ACK;
		}
		else
		{
			fprintf(stderr, "INCOMPLETE PACKET of Length: %d", current_packet);
		}

	} while (current_packet > 0);

	return newdata_received;
}

std::vector<Frame> SenteraDouble4k::Data() {
	std::vector<Frame> outFrames;
	for (int i = 0; i < num_cameras; i++) {
		updated[i] = false;
		outFrames.push_back(sensor_data[i]);
	}
	return outFrames;
}

int SenteraDouble4k::processImage(int cam) {
	// make URL string to grab data from, then grab the data
	std::string urlStr = makeUrlPath(recent_images[cam-1].fileName);
	std::string imgContent = http_downloader.download(urlStr);

	// debug
	//std::cout << "Successfully downloaded sentera's .jpg from: " << urlStr << ", Length: " << imgContent.length() << "\n";

	size_t compressedImgLength = imgContent.length();
	unsigned char *compressedImg = (unsigned char*)imgContent.c_str();

	std::string outname = "";
	for (int i = 0; i < 48; i++) { // filename array size 48
		outname += (const char)recent_images[cam-1].fileName[i];
	}
	outname += ".jpg";

	try {
		std::ofstream outfile(outname, std::ofstream::binary);
		outfile.write(imgContent.c_str(), compressedImgLength);
		// check if successful
		if ((outfile.rdstate() & std::ofstream::failbit) != 0) {
			fprintf(stderr, "Error saving sentera jpg locally to %s... Camera: %d: %s\n", outname.c_str(), cam, strerror(errno));
		}
		else {
			fprintf(stderr, "Successfully saved downloaded sentera .jpg locally to %s, Camera ID: %d\n", outname.c_str(), cam);
		}
	}
	catch (std::ofstream::failure const &ex) {
		fprintf(stderr, "Caught exception attempting to save sentera jpg locally to %s, camera %d: %s", outname.c_str(), cam, ex.what());
	}


	

	// initialize variables to fill with data
	int width, height;
	int channels = 3; // 3 channels for RGB data

	// initialize decompressor and get size
	// tjhandle _jpegDecompressor = tjInitDecompress(); // called in constructor
	int resultCode = tjDecompressHeader(_jpegDecompressor, compressedImg, compressedImgLength, &width, &height);
	if (resultCode == -1) { fprintf(stderr, "jpeg header decompression failed"); }
	size_t size = width * height * channels; 

	//std::cout << "About to attempt to allocate unsigned char array to hold decompressed .jpg of size: " << size << " (" << width << " x " << height << ")\n";
	try {
		if (sensor_data[cam - 1].pixels)	delete[] sensor_data[cam - 1].pixels; // free up memory from old image
		sensor_data[cam - 1].pixels = new unsigned char[size]; // allocate new memory for incoming data
	} catch (std::bad_alloc ba) {
		fprintf(stderr, "Heap allocation failed attempted to create buffer to hold uncompressed jpeg of size %d", size);
		return -1;
	}

	// decompress the jpg
	resultCode = tjDecompress2(_jpegDecompressor, compressedImg, compressedImgLength, sensor_data[cam-1].pixels, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);
	if (resultCode == -1) { fprintf(stderr, "jpeg body decompression failed"); }

	// debug
	//std::cout << "Successfully decompressed .jpg\n";
																				  
	// deal with buffer
	sensor_data[cam - 1].width = width;
	sensor_data[cam - 1].height = height;
	sensor_data[cam - 1].bands = channels;
	updated[cam - 1] = true;
	return 0;
}

std::string SenteraDouble4k::makeUrlPath(uint8_t *filename) {
	// http ://192.168.143.141:8080/sdcard?cur_session&path=/RGB/IMG_000001.jpg
	std::string outStr = "http://";
	outStr += server_ipaddr;
	outStr += ":";
	outStr += "8080";
	outStr += "/sdcard?cur_session&path=/";
	for (int i = 0; i < 48; i++) { // filename array size 48
		outStr += (const char)filename[i];
	}
	return outStr;
}

// sendNDVI
// calculates NDVI, saves locally as jpg and sends location to hololens
void SenteraDouble4k::sendNDVI(int quality) {
	// get width, height and filename
	int width = sensor_data[0].width; // doesnt have to be!! can choose to send any size image. will resample and scale accordingly
	int height = sensor_data[0].height;
	char filename[IMG_FILENAME_LEN];
	for (int i = 0; i < IMG_FILENAME_LEN - IMG_FILENAME_DIR_LEN; i++) {
		filename[i] = (const char)recent_images[1].fileName[i + IMG_FILENAME_DIR_LEN];
	}

	// debug
	//fprintf(stderr, "Preparing NDVI image: %s\n", filename);

	// fill NDVI buffer
	uint8_t *ndvibuf;
	try {
		ndvibuf = new uint8_t[width * height];
	} catch (std::bad_alloc ba) {
		fprintf(stderr, "Heap allocation failed attempted to create buffer to hold NDVI data of size %d", width * height);
		return;
	}
	DataProcessor::getSenteraNDVI(sensor_data, width, height, ndvibuf);

	// debug
	//fprintf(stderr, "Got unprocessed NDVI data: %s\n", filename);

	// save un-processed NDVI image locally
	int unprocessedQuality = 100;
	uint8_t* jpegBuf = nullptr;
	int jpegSize;
	try {
		jpegSize = compressor.compressBandJpeg(ndvibuf, &jpegBuf, width, height, unprocessedQuality);
	}
	catch(std::exception ex) {
		fprintf(stderr, "Caught exception attempting to compress full-size NDVI to jpg: %s", ex.what());
	}

	// debug
	//fprintf(stderr, "Compressed unprocessed NDVI data to jpg: %s\n", filename);

	std::string filename_string(filename);
	std::string outname = "NDVI_FULL/" + filename_string;
	try {
		std::ofstream outfile(outname, std::ofstream::binary);
		outfile.write(reinterpret_cast<const char*> (jpegBuf), width*height);
		// check if successful
		if ((outfile.rdstate() & std::ofstream::failbit) != 0) {
			fprintf(stderr, "Error saving full NDVI jpg locally to %s: %s\n", outname.c_str(), strerror(errno));
		}
		else {
			//fprintf(stderr, "Saved NDVI image locally as: %s\n", outname.c_str());
		}
	}
	catch (std::ofstream::failure const &ex) {
		fprintf(stderr, "Caught exception attempting to save full NDVI jpg locally to %s: %s", outname.c_str(), ex.what());
	}

	// debug
	//fprintf(stderr, "Saved unprocessed jpg: %s\n", filename);

	// process NDVI img
	uint8_t *processed_ndvibuf;
	Vector3Int processedSize;
	Vector3Int unprocessSize(width, height, 1);
	switch (PROCESS_MODE) {
		case none: 
			processedSize = unprocessSize;
			processed_ndvibuf = ndvibuf; 
			break;
		case halfSample: 
			try {
				processed_ndvibuf = new uint8_t[width / 2 * height / 2];
				DataProcessor::HalfSample(ndvibuf, processed_ndvibuf, unprocessSize, &processedSize);
			}
			catch (std::exception ex) {
				fprintf(stderr, "Caught exception attempting to halfSample ndvi: %s", ex.what());
			} 
			break;
	}

	// debug
	//fprintf(stderr, "Processed NDVI data: %s\n", filename);

	// save processed jpg locally
	uint8_t* processed_jpegBuf = nullptr;
	int processed_jpegSize;
	try {
		processed_jpegSize = compressor.compressBandJpeg(processed_ndvibuf, &processed_jpegBuf, processedSize.x, processedSize.y, quality);
	}
	catch (std::exception ex) {
		fprintf(stderr, "Caught exception attempting to compress processed NDVI to jpg: %s", ex.what());
	}

	// debug
	//fprintf(stderr, "Compressed processed NDVI data to jpg: %s\n", filename);
	
	outname = "NDVI/" + filename_string;
	try {
		std::ofstream outfile2(outname, std::ofstream::binary);
		outfile2.write(reinterpret_cast<const char*> (processed_jpegBuf), processedSize.x * processedSize.y);
		// check if successful
		if ((outfile2.rdstate() & std::ofstream::failbit) != 0) {
			fprintf(stderr, "Error saving processed NDVI jpg locally to %s: %s\n", outname.c_str(), strerror(errno));
		}
		else {
			//fprintf(stderr, "Saved processed NDVI image locally as: %s\n", outname.c_str());
		}
	}
	catch (std::ofstream::failure const &ex) {
		fprintf(stderr, "Caught exception attempting to save processed NDVI jpg locally to %s: %s", outname.c_str(), ex.what());
	}

	// debug
	fprintf(stderr, "Processed ndvi img (%s), Process mode: %s. Was %d bytes (%dx%d, q=%d), Now %d bytes (%dx%d, q=%d)\n", 
		filename, PROCESS_MODE_NAMES[PROCESS_MODE], jpegSize, unprocessSize.x, unprocessSize.y, unprocessedQuality, 
		processed_jpegSize, processedSize.x, processedSize.y, quality);

	// transmit processed NDVI jpg
	int messageLen;
	switch (TRANS_MODE) {
		case fullFile: messageLen = trans.HEADER_SIZE + processed_jpegSize; break;
		case fileName: messageLen = trans.HEADER_SIZE + IMG_FILENAME_LEN; break;
	}
	 
	uint8_t *transBuf;
	try {
		transBuf = new uint8_t[messageLen];
	}
	catch (std::bad_alloc ba) {
		fprintf(stderr, "Heap allocation failed attempted to create buffer to hold transmission message of size %d", messageLen);
		return;
	}

	// add in header: length, img width, img height
	Serializer::serializeInt(transBuf, messageLen);
	Serializer::serializeInt(transBuf + 4, width);
	Serializer::serializeInt(transBuf + 8, height);

	// add in data
	try {
		switch (TRANS_MODE) {
			case fullFile: std::memcpy(transBuf + 12, processed_jpegBuf, processed_jpegSize); break;
			case fileName: std::memcpy(transBuf + 12, filename, IMG_FILENAME_LEN); break;
		}
	}
	catch (std::exception ex) {
		fprintf(stderr, "Caught exception attempting to copy processed jpg to transmission buffer: %s", ex.what());
	}
	

	// transmit
	fprintf(stderr, "Attempting transmission to Hololens (%s)... Trans. mode: %s, Trans. size (bytes): %d\n\n", 
		filename, TRANS_MODE_NAMES[TRANS_MODE], messageLen);
	if (trans.hasConnection()) {
		trans.transmit((char*)transBuf, messageLen);
	}

	delete[] ndvibuf;
	delete[] processed_jpegBuf;
	delete[] transBuf;

	// debug
	//printf("Transmitted NDVI Image jpg\n\n");
}

