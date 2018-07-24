#include "sentera.h"
#include <iostream>
// #include <cpprest/http_client.h>

// using namespace web::http;                  // Common HTTP functionality
// using namespace web::http::client;

sentera::sentera() {
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset((char *)&address, 0, sizeof(address));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(IP); // this is address of host which I want to send the socket
	address.sin_port = htons(atoi(PORT));


	printf("handle: %d\n", sock); // prints number greater than 0 so I assume handle is initialized properly

	if (bind(sock, (const struct sockaddr*) &address, sizeof(struct sockaddr_in)) < 0)
	{
		printf("failed to bind socket (%s)\n", strerror(errno)); // Cannot assign requested address
	}

  sessionName = std::string("test");
}

sentera::~sentera() {

}

// Send start capture command
void sentera::startCapture() {
  uint16_t payload_length = 142;
  char payload[payload_length];
  for (uint16_t i = 0; i < payload_length; ++i) {
    payload[i] = (char)0;
  }
  int ind = 0;
  while ((ind++) < sessionName.size()) {
    payload[ind] = sessionName.at(ind - 1);
  }
  payload[ind] = '\0'; // null terminate string
  char *packet = assemblePacket(SET_STILL_CAPTURE, payload_length, payload);
  uint16_t total_length = payload_length + 2 + 1 + 2 + 1;
  int ret = sendto(sock, (char *)packet, total_length, 0, (const struct sockaddr*) &address, sizeof(struct sockaddr_in));
  delete[] packet;
}

// Return false if no data available yet, true otherwise
bool sentera::getCaptureUrl(char *urlBuf) {
  char buf[1000];
  recvfrom(sock, buf, 1000, 0, nullptr, nullptr);
  uint8_t type = buf[2];
  if (type == GET_IMAGE_DATA_READY) {
    parseForUrl(buf, urlBuf);
    return true;
  } else {
    return false;
  }
}

void sentera::parseForUrl(char* buf, char *urlBuf) {
  for (int i = 0; i < IMAGE_URL_LENGTH; ++i) {
    urlBuf[i] = buf[i + 1];
  }
}

// Using url, grab the image then transmit it
/*
void sentera::getImageAndTransmit(std::string requestString) {
  http_client client(requestString);
  http_response response;
  response = client.request(methods::GET).get(); // unsure of arguments to this request method

  char *image = new char[response.extract_string().get().size()];
  for (int i = 0; i < response.extract_string().get().size(); ++i) {
    image[i] = response.extract_string().get().at(i);
  }

  // transmitter.transmitRGBPreCompressed(image, response.extract_string.size());
}*/

std::string sentera::createRequestString(char* urlBuf) {
  std::string requestString("http://");
  requestString += IP;
  requestString += ":";
  requestString += PORT;
  requestString += "/";
  requestString += sessionName;
  requestString += "?path=/";
  requestString += urlBuf;
  return requestString;
}

// Capture and transmit loop
void sentera::startCaptureAndTransmit() {
  while (1) {
    startCapture();
    char urlBuf[IMAGE_DATA_READY_LENGTH];
    while (!getCaptureUrl(urlBuf));
    std::string requestString = createRequestString(urlBuf);
    std::cout << requestString << std::endl;
    // getImageAndTransmit(requestString);
  }
}

char* sentera::assemblePacket(uint8_t type, uint16_t length, char *payload) {
  // payload length + 2 headers + 2 byte length + crc + type
  char *packet = new char[length + 2 + 2 + 1 + 1];
  packet[0] = HEADER_ZERO;
  packet[1] = HEADER_ONE;
  packet[2] = type;
  packet[3] = (length & 0xff);
  packet[4] = (length >> 8) & 0xff;


  for (int i = 0; i < length; ++i) {
    packet[5 + i] = payload[i];
  }

  uint8_t crc = calcCRC(&(packet[2]), 3 + length);

  packet[length + 2 + 2 + 1] = crc;
  return packet;
}

// You need to pass in the correct address to ar
uint8_t sentera::calcCRC(char *arStart, int length) {
  uint8_t crc = 0;
  for(int i = 0; i < length; ++i) {
    uint8_t b = arStart[i];
    for (int bit = 7; bit >= 0; --bit) {
      if ((crc & 0x80) != 0) {   /* MSB set, shift it out of the register */
          crc = (uint8_t)(crc << 1);
          /* shift in next bit of input stream:
           * If it's 1, set LSB of crc to 1.
           * If it's 0, set LSB of crc to 0. */
          crc = ((uint8_t)(b & (1 << i)) != 0) ? (uint8_t)(crc | 0x01) : (uint8_t)(crc & 0xFE);
          /* Perform the 'division' by XORing the crc register with the generator polynomial */
          crc = (uint8_t)(crc ^ GENERATOR_POLYNOMIAL);
      } else {   /* MSB not set, shift it out and shift in next bit of input stream. Same as above, just no division */
          crc = (uint8_t)(crc << 1);
          crc = ((uint8_t)(b & (1 << i)) != 0) ? (uint8_t)(crc | 0x01) : (uint8_t)(crc & 0xFE);
      }
    }
  }
  return crc;
}
