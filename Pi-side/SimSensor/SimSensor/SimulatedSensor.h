// Simulated sensor for testing
// Mark Scherer, July 2018

#pragma once
#ifndef SIMSENSOR_H
#define SIMSENSOR_H

#include "Sensor.h" 

class SimSensor : public Sensor {
public:
	// constructor
	SimSensor(Transform offset, int _height, int _width, int _bands, float _FOVx, float _FOVy)
		: Sensor(offset), height(_height), width(_width), bands(_bands), FOVx(_FOVx), FOVy(_FOVy)
	{
		// nothing to do
	}

	// copy constructor
	SimSensor(const SimSensor &rhs)
		: Sensor(rhs.offset), height(rhs.height), width(rhs.width), bands(rhs.bands), FOVx(rhs.FOVx), FOVy(rhs.FOVy)
	{
		// nothing to do
	}

	// assignment op
	SimSensor& operator=(const SimSensor &rhs) {
		SimSensor tmp(rhs);
		std::swap(tmp, *this);
	}

	// data accessor
	Frame Data() {
		// refresh sensor's data randomly
		delete data;
		data = new Frame(height, width, bands, FOVx, FOVy);
		for (int i = 0; i < height*width*bands; i++) {
			data->pixels[i] = (uint8_t)rand();
			//data->pixels[i] = i;
		}
		return *data;
	}

	// start/stop sensor recording
	void Start() {
		updated = true;
	}

	void Stop() {
		// do nothing
	}

private:
	int height, width, bands;
	float FOVx, FOVy;
};

# endif // SIMSENSOR_H