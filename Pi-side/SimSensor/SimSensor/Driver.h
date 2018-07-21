// Driver
// Transmits new sensor data
// configured for sensor: SimSensor (SimulatedSensor.h)
// Mark Scherer, July 2018

#pragma once
#ifndef DRIVER
#define DRIVER

#include "SimulatedSensor.h"
#include "Transmitter.h"

class Driver {
public:
	Driver(Transform offset, int height, int width, int bands, float FOVx, float FOVy,
		const char* localIP, int inPort, int outPort)
		: sensor(offset, height, width, bands, FOVx, FOVy), 
		  sender(Transmitter(localIP, inPort, outPort))
	{
		// nothing to do	
	}

	void go() {
		while (true) {
			sendFrame();
		}
	}

private:
	SimSensor sensor;
	Transmitter sender;

	void sendFrame() {
		if (sensor.getUpdated()) {
			Frame sensorData = sensor.Data();
			sender.transmit(sensorData.pixels, sensorData.height * sensorData.width * sensorData.bands);
		}
	}
};

#endif // DRIVER

