#include "transmit.h"

static int PACKET_SIZE = 1000; // anticipate + 28 packet size for headers

transmit::transmit(const char* remoteIp, const char* remotePort) {

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset((char *)&address, 0, sizeof(address));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(remoteIp); // this is address of host which I want to send the socket
	address.sin_port = htons(atoi(remotePort));


	printf("handle: %d\n", sock); // prints number greater than 0 so I assume handle is initialized properly

	if (bind(sock, (const struct sockaddr*) &address, sizeof(struct sockaddr_in)) < 0)
	{
		printf("failed to bind socket (%s)\n", strerror(errno)); // Cannot assign requested address
	}


	isTransmitting.store(true);
	transmitThread = std::thread(&transmit::checkTransmissionBuffer, this);
}


transmit::~transmit()
{
	close(sock);
	isTransmitting.store(false);
	bufferCv.notify_all();
	transmitThread.join();
}

void transmit::transmitRGBImage(uchar* uncompressedImage, int width, int height, int quality) {
	uchar* jpegBuf = nullptr;
	// uchar *uncompressedAr = compressor.getArrayFromMat(image);
	int jpegSize = compressor.compressRGBJpeg(uncompressedImage, &jpegBuf, width, height, quality);
	// indicate bgr with M
	loadBuffer(jpegBuf, jpegSize, 'M', true);
}

void transmit::transmitRGBPreCompressed(uchar *compressedImage, int size) {
	loadBuffer(compressedImage, size, 'M', false);
}

void transmit::transmitImage(uchar* uncompressedImage, int width, int height, int quality) {
	uchar* jpegBuf = nullptr;
	int jpegSize = compressor.compressBandJpeg(uncompressedImage, &jpegBuf, width, height, quality);
	// Indicate 1-band image with I
	loadBuffer(jpegBuf, jpegSize, 'I', true);
}

void transmit::transmitImagePreCompressed(uchar* compressedImage, int size) {
	loadBuffer(compressedImage, size, 'I', false);
}

void transmit::sendDone() {
	char done = char{ 'D' };
	int ret = sendto(sock, &done, 1, 0, (const struct sockaddr*) &address, sizeof(struct sockaddr_in));
}

void transmit::doTransmit(struct packet* transmissionPacket) {
	int len = transmissionPacket->packetSize;
	int ret, response;
	uint16_t num_packets = (len + (PACKET_SIZE - 1)) / PACKET_SIZE;

	int numBytesPacketIndex = 3;
	len = len + (numBytesPacketIndex * num_packets);

	std::cout << "num_packets: " + num_packets;
	const int initial_transmission_length = 3;
	char initial_transmission[initial_transmission_length];
	initial_transmission[0] = transmissionPacket->type; // First char received at the start of a transmission, it's necessary in order to determine RGB or 1-band
	initial_transmission[1] = (char)(num_packets & 0xff); // Get the lower byte
	initial_transmission[2] = (char)(((num_packets) >> 8) & 0xff); // Get the upper byte

	// Index + size transmission are little endian

	// Send the first char (start char + indicates what type of image)
	ret = sendto(sock, initial_transmission, initial_transmission_length, 0, (const struct sockaddr*) &address, sizeof(struct sockaddr_in));

	uchar* transmission_data = new uchar[len]; // We may have over 256 packets ergo can't use 1 byte. We actually use 3 bytes for the packet index because it lets us
	// distinguish image data from start of transmission packet

	int cur_ind = 0;
	for (int i = 0; i < num_packets; ++i) {
		for (int byte = 0; byte < numBytesPacketIndex; ++byte) {
			// Add the bytes to indicate the index of the packet
			transmission_data[cur_ind++] = ((i >> (byte * 8)) & 0xff);
		}
		for (int j = i * PACKET_SIZE; j < (i + 1) * PACKET_SIZE && j < transmissionPacket->packetSize; ++j) {
			int ind = j + ((i + 1) * numBytesPacketIndex);
			transmission_data[cur_ind++] = transmissionPacket->jpegBuf[j];
		}
	}

	if (transmissionPacket->isTjCompressed) {
		tjFree(transmissionPacket->jpegBuf);
	} else {
		delete[] transmissionPacket->jpegBuf;
	}
	// Need to free transission_data to avoid using too much memory....
	delete transmissionPacket;


	int packetSize = PACKET_SIZE + numBytesPacketIndex; // Need to include index of packet (it's at the start of each packet)

	char* startPtr, *charBuf;
	charBuf = reinterpret_cast<char *>(transmission_data);
	startPtr = charBuf;

	// Check to make sure we still have a packet to send (e.g. if buffer = 6, packet size = 2, then keep transmitting
	// until, in this case, startPtr - charBuf = 1, at which point we'll have one last packet to send, which we do outside the loop)
	while (startPtr - charBuf < len - packetSize) {
		ret = sendto(sock, startPtr, packetSize, 0, (const struct sockaddr*) &address, sizeof(struct sockaddr_in));
		if (ret < 0) {
			// throwLastError("transmit failed");
	}
		startPtr += packetSize;
	}

	// std::cout << "finished image";

	// last packet transmission
	ret = sendto(sock, startPtr, (charBuf + len) - startPtr, 0, (const struct sockaddr*) &address, sizeof(struct sockaddr_in));
	if (ret < 0) {
		throwLastError("transmit failed");
	};
	// empty packet to indicate termination

	ret = sendto(sock, (char *)0, 0, 0, (const struct sockaddr*) &address, sizeof(struct sockaddr_in));

	delete[] transmission_data;

}

void transmit::throwLastError(std::string errorLabel){
	std::cout <<  strerror(errno);
	throw std::system_error(errno, std::system_category(), errorLabel);
}

void transmit::checkTransmissionBuffer() {
	while (isTransmitting.load()) {
		std::unique_lock<std::mutex> lk(bufferLock);
		while (bufferFront == bufferBack) {
			bufferCv.wait(lk);
			if (!isTransmitting.load()) {
				throw "transmission ended";
			}
		}
		struct packet* transmission = imageBuffer[bufferFront];
		bufferFront = (bufferFront + 1) % IMAGE_BUFFER_LEN;
		// transmit from buffer
		--size;
		bufferCv.notify_all();
		doTransmit(transmission);
	}
}

void transmit::loadBuffer(uchar* jpegBuf, int jpegSize, char type, bool isTjCompressed) {
	struct packet* transmissionPacket = new packet();
	transmissionPacket->jpegBuf = jpegBuf;
	transmissionPacket->type = type;
	transmissionPacket->packetSize = jpegSize;
	transmissionPacket->isTjCompressed = isTjCompressed;

	std::unique_lock<std::mutex> lk(bufferLock);
	while ((bufferBack + 1) % IMAGE_BUFFER_LEN == bufferFront) {
		bufferCv.wait(lk);
	}
	// load into the buffer
	imageBuffer[bufferBack] = transmissionPacket;
	bufferBack = (bufferBack + 1) % IMAGE_BUFFER_LEN;
	++size;
	bufferCv.notify_all();
}

void transmit::emptyBuffer() {
	std::unique_lock<std::mutex> lk(bufferLock);
	lk.lock();
	bufferFront = bufferBack = 0;
	lk.unlock();
}
