// Header for Sentera Camera Class
// Henry Bristol
// July 25, 2018

// includes
#include "SenteraDouble4k.h"
#include <fstream>

SenteraDouble4k::SenteraDouble4k() : SenteraDouble4k::SenteraDouble4k(offset){}

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
		printf("Successfully Setup Sockets!\n"); //DEBUG
	}
}

SenteraDouble4k::~SenteraDouble4k()
{
	close(s_send);
	close(s_rec);
	tjDestroy(_jpegDecompressor);
}

int SenteraDouble4k::Start() {
	const char currentSessionName[128] = "TestingName1";
	if (serv_status == -1) {
		printf("Server not initialized! Cannot start session.");
		return -1;
	}
	
	// initialize packet to fill and send
	uint8_t buf[BUFLEN];

	// make packet of still capture session data 
	int packet_length = makeStillCapturePacket((uint8_t)0, currentSessionName, buf);
	if (packet_length <= 0) {
		printf("Failed to create packet");
		return -1;
	}
	printf("Still Capture Packet Created. Length: %d, Type: %X\n", packet_length, buf[2]); 	//DEBUG

	// send packet of still capture session data
	if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1) {
		printf("Failed to send packet: %d", errno); //DEBUG
		return -1;
	}
	//DEBUG printf("Sent still capture packet");
	printf("Started and listening for data!\n");
	return sessionListener();

	//return 0;
}

int SenteraDouble4k::sessionListener() {
	// listen for updates
	live_session = true;
	int recvType = 0;

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
				// debug
				std::cout << " Received data from camera, processing...\n";

				//DEBUG printf("Images ready for Camera %d\n", imgReadyID);
				processImage(imgReadyID); // process data for appropriate image
				//DEBUG printf("Images processed for Camera %d\n", imgReadyID);
				//DEBUG printf("Bands Filtered for Camera %d\n", imgReadyID);
				if (imgReadyID == 2 && getUpdated()) sendNDVI(80); // send NDVI image each time NIR data is received
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
		printf("Failed to create packet");
		return -1;
	}

	// send packet of still capture session data
	if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1) {
		printf("Failed to send packet: %d", errno); //DEBUG
		return -1;
	}

	live_session = false;
	return 0;
}

int SenteraDouble4k::updateTrigger() {
	//check server
	if (serv_status == -1) {
		printf("Server not initialized! Cannot start session.");
		return -1;
	}

	// initialize packet to fill and send
	uint8_t buf[BUFLEN];

	// make packet of trigger data 
	int packet_length = makeImagerTriggerPacket((uint8_t)2, (uint32_t)1000, buf);
	if (packet_length <= 0) {
		printf("Failed to create packet");
		return -1;
	}
	//printf("Trigger Packet Created. Length: %d, Type: %X\n", packet_length, buf[2]); 	//DEBUG

	//send packet of trigger data
	if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send) == -1) {
		printf("Failed to send packet: %d", errno); //DEBUG
		return -1;
		return -1;
	}
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
			printf("Recv Packet Header Failure");
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
			printf("Received Imager Trigger Acknowledgement\n");
			newdata_received = fw_packet_type_e::IMAGER_TRIGGER_ACK;
		}
		else
		{
			printf("INCOMPLETE PACKET of Length: %d", current_packet);
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
	std::cout << "Successfully downloaded sentera's .jpg from: " << urlStr << ", Length: " << imgContent.length() << "\n";

	size_t compressedImgLength = imgContent.length();
	unsigned char *compressedImg = (unsigned char*)imgContent.c_str();

	std::string outname = "";
	for (int i = 0; i < 48; i++) { // filename array size 48
		outname += (const char)recent_images[cam-1].fileName[i];
	}
	//outname += ".jpg";
	//printf("Wrote: %s\n", outname.c_str());
	std::ofstream outfile(outname , std::ofstream::binary);
	outfile.write(imgContent.c_str(), compressedImgLength);

	// debug
	std::cout << "Successfully saved downloaded .jpg locally to " << outname << "\n";

	// initialize variables to fill with data
	int width, height;
	int channels = 3; // 3 channels for RGB data

	// initialize decompressor and get size
	// tjhandle _jpegDecompressor = tjInitDecompress(); // called in constructor
	int resultCode = tjDecompressHeader(_jpegDecompressor, compressedImg, compressedImgLength, &width, &height);
	if (resultCode == -1) { std::cout << "jpeg header decompression failed.\n"; }
	size_t size = width * height * channels; 

	std::cout << "About to attempt to allocate unsigned char array to hold decompressed .jpg of size: " << size << " (" << width << " x " << height << ")\n";
	try {
		if (sensor_data[cam - 1].pixels)	delete[] sensor_data[cam - 1].pixels; // free up memory from old image
		sensor_data[cam - 1].pixels = new unsigned char[size]; // allocate new memory for incoming data
	} catch (std::bad_alloc ba) {
		std::cout << "Allocation failed, not enough space on heap...\n";
		return -1;
	}

	

	// decompress the jpg
	resultCode = tjDecompress2(_jpegDecompressor, compressedImg, compressedImgLength, sensor_data[cam-1].pixels, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);
	if (resultCode == -1) { std::cout << "jpeg body decompression failed.\n"; }

	// debug
	std::cout << "Successfully uncompressed .jpg\n";
																				  
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

void SenteraDouble4k::sendNDVI(int quality) {
	// buffer for NDVI data
	//image width and height
	int width = sensor_data[0].width; // doesnt have to be!! can choose to send any size image. will resample and scale accordingly
	int height = sensor_data[0].height;

	// debug
	std::cout << "About to create buffer to hold NDVI... Length: " << width * height << " (" << width << " x " << height << ")\n";

	// fill NDVI buffer
	std::cout << "About to attempt to allocate uint8_t buffer to hold NDVI values of size: " << width * height << "\n";
	uint8_t *ndvibuf;
	try {
		ndvibuf = new uint8_t[width * height];
	} catch (std::bad_alloc ba) {
		std::cout << "Allocation failed, not enough space on heap...\n";
		return;
	}
	

	// debug
	printf("Pre-downsampled NDVI Buffer made of size %d\n", width*height);

	DataProcessor::getSenteraNDVI(sensor_data, width, height, ndvibuf);

	// debug
	printf("Filled NDVI data buffer\n");

	// save jpeg locally
	uint8_t* jpegBuf = nullptr;
	int jpegSize = compressor.compressBandJpeg(ndvibuf, &jpegBuf, width, height, 100);
	std::string outname = "NDVI/";
	for (int i = 5; i < 48; i++) { // filename array size 48, ignore first folder
		outname += (const char)recent_images[1].fileName[i];
	}
	outname += ".jpg";
	std::ofstream outfile(outname, std::ofstream::binary);
	outfile.write(reinterpret_cast<const char*> (jpegBuf), width*height);

	// debug
	printf("Wrote: %s\n", outname.c_str());

	// resample and transmit uncompressed buffer
	// create buffer
	int resampWidth = 20;
	int resampHeight = ((float)height / (float)width) * (float)resampWidth;
	int messageLen = resampWidth * resampHeight + trans.HEADER_SIZE;

	std::cout << "About to attempt to allocate uint8_t buffer to hold outbound transmission of size: " << messageLen << "\n";
	uint8_t *transBuf;
	try {
		transBuf = new uint8_t[messageLen];
	} catch (std::bad_alloc ba) {
		std::cout << "Allocation failed, not enough space on heap...\n";
		return;
	}
	

	// debug
	std::cout << "created downsampled buffer\n";

	// add in header
	Serializer::serializeInt(transBuf, messageLen);
	Serializer::serializeInt(transBuf + 4, resampWidth);
	Serializer::serializeInt(transBuf + 8, resampHeight);

	// fill in NDVI data
	DataProcessor::Resample(ndvibuf, Vector3Int(width, height, 1), Vector3Int(resampWidth, resampHeight, 1), transBuf + 12);
	
	// debug
	std::cout << "Resampled NDVI buf to " << resampWidth << " x " << resampHeight << ". Data length: " << resampWidth * resampHeight << " bytes.\n";

	// transmit
	if (trans.hasConnection()) {
		trans.transmit((char*)transBuf, messageLen);
	}

	//printf("Saved NDVI image to:%s\n", "1");
	//printf("Downsampled image 2x2: Length %d\n", width / 2 * height / 2);
	//transmitter.transmitImage(resampleBuf, width/2, height/2, quality);
	delete[] ndvibuf;
	delete[] transBuf;

	// debug
	printf("Transmitted downsampled NDVI Image\n");
}

