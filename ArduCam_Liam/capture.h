#pragma once

#include <string.h>
#include <time.h>
#include "transmit.h"
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include "arducam_arch_raspberrypi.h"

#define OV2640_CHIPID_HIGH 	0x0A
#define OV2640_CHIPID_LOW 	0x0B
#define OV2640_MAX_FIFO_SIZE		0x5FFFF			//384KByte
#define BUF_SIZE 4096
#define CAM1_CS 5

/*
 * Please note: this class has been written specifically for the OV2640. It will
 * be abstracted for any sensor later.
*/
class capture
{

public:
	capture();
	~capture();

  void startCaptureAndTransmit();

  void setImageSize();

private:

  transmit transmitter;
  ArduCAM myCAM;

};
