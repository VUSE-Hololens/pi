#include "opencv2/opencv.hpp"

#include "sys/socket.h"
#include <arpa/inet.h>
#include <netinet/in.h> /* Needed for sockaddr_in */
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <errno.h> /* Needed for error handling */
#include <fcntl.h>
#define INVALID_SOCKET (-1) /* defined in WinSock2.h but not *nix */

#include <system_error>
#include "compress.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <atomic>

#define RECIPIENT_IP "10.66.108.2"
#define PORT "5000"
#define IMAGE_BUFFER_LEN 6


// static int PACKET_SIZE = 228; // total size (for server receiving) = packet header + ip header + data

struct packet {
	uchar* jpegBuf;
	char type;
	int packetSize;
	bool isTjCompressed;
};

class transmit
{
public:

	transmit(const char* ip=RECIPIENT_IP, const char* port=PORT);
	~transmit();

	void transmitRGBImage(uchar* uncompressedImage, int width, int height, int quality);
	void transmitRGBPreCompressed(uchar *compressedImage, int size);

	void transmitImage(uchar* uncompressedImage, int width, int height, int quality);
  void transmitImagePreCompressed(uchar* compressedImage, int size);

	void sendDone();
	void emptyBuffer();


private:

	int sock;

	compress compressor;
	std::string myPort;
	std::string myIp;

	struct sockaddr_in address;

	std::thread transmitThread;
	std::mutex bufferLock;
	std::condition_variable bufferCv;

	std::atomic<bool> isTransmitting;

	struct packet* imageBuffer[IMAGE_BUFFER_LEN]; // circular queue for images
	int bufferFront = 0;
	int bufferBack = 0;
	int size = 0; // may or may not be useful

	void doTransmit(struct packet* transmissionPacket);

	void checkTransmissionBuffer();
	void loadBuffer(uchar* jpegBuf, int jpegSize, char type, bool isTjCompressed);
	void throwLastError(std::string errorLabel);
};
