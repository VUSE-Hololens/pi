#include <stdio.h>
#include "sys/socket.h"
#include <arpa/inet.h>
#include <netinet/in.h> /* Needed for sockaddr_in */
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <errno.h> /* Needed for error handling */
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>

#define IP "192.168.143.141"
#define PORT "8080"

#define HEADER_ZERO 0x46
#define HEADER_ONE 0x57
#define GENERATOR_POLYNOMIAL 0x87 // This might have to be 0x07??
#define IMAGE_DATA_READY_LENGTH 53
#define IMAGE_URL_LENGTH 48

// PACKET HEADERS - TO PAYLOAD
#define SET_AIRCRAFT_MEDATADA 0x01 // To Payload Aircraft Metadata
#define SET_IMAGER_TRIGGER 0x02 // To Payload Imager Trigger
#define SET_IMAGER_POWER 0x03 // To Payload Imager Power
#define SET_STILL_CAPTURE 0x04 // To Payload Still Capture Session
#define SET_VIDEO_CAPTURE 0x05 // To Payload Video Session
#define SET_VIDEO_FOCUS 0x06 // To Payload Video Focus Session
#define SET_STILL_FOCUS 0x07 // To Payload Still Focus Session
#define SET_AUTOPILOT_PING_RESPONSE 0x08 // To Payload Autopilot Ping Response
#define SET_VIDEO_SESSION_ADVANCED 0x09 // To Payload Video Session Advanced
#define SET_VIDEO ADJUST 0x0A // To Payload Video Adjust
#define SET_PAYLOAD_RESERVED 0x0B // To Payload(Reserved)
#define SET_IMAGER_ZOOM 0x0C // To Payload Imager Zoom
#define SET_IMAGER_PREVIEW_STREAM_SETUP 0x0D // To Payload Imager Preview Stream Setup
#define SET_ELEVATION_METADATA 0x0E // To Payload Elevation Metadata
#define SET_SYSTEM_TIME 0x0F // To Payload System Time
#define SET_VIDEO_ADJUST_RELATIVE 0x10 // To Payload Video Adjust Relative

// PACKET HEADERS - FROM PAYLOAD
#define GET_PAYLOAD_METADATA 0x81 // From Payload Payload Metadata
#define GET_IMAGER_TRIGGER_ACK 0x82 // From Payload Imager Trigger Ack
#define GET_STILL_CAPTURE_SESSION_ACK 0x83 // From Payload Still Capture Session Ack
#define GET_AUTOPILOT_PING_REQUEST 0x84 // From Payload Autopilot Ping Request
#define GET_IMAGE_DATA_READY 0x85 // From Payload Image Data Ready
#define GET_SYSTEM_TIME_ACK 0x8F // From Payload System Time Ack
#define GET_FOCUS_SCORE 0xD0 // From Payload Focus Score
#define GET_PAYLOAD_EXCEPTION 0xFF // From Payload Payload Exception

class sentera {
public:
  sentera();
  ~sentera();
  void startCaptureAndTransmit();
private:
  void startCapture();
  bool getCaptureUrl(char *urlBuf);
  // void getImageAndTransmit(std::string requestString);
  uint8_t calcCRC(char *arStart, int length);
  std::string createRequestString(char* urlBuf);
  void parseForUrl(char* buf, char *urlBuf);

  char* assemblePacket(uint8_t type, uint16_t length, char *payload);

  // transmit transmitter;
  struct sockaddr_in address;
  int sock;

  std::string sessionName;
};
