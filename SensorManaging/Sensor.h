// Sensor.h
// Abstract class for sensors
// Mark Scherer, July 2018

#pragma once
#ifndef SENSOR_H
#define SENSOR_H

#include <memory>
#include <string>
#include <vector>

// user include
#include "SensorDataTypes.h"
#include "SensorDataTypes.h"

// abstract class for all sensor inputs
class Sensor {

protected:
	// static IP address of Raspberry Pi
	const std::string PI_IP = "192.168.143.130";

public:
	// constructor
	Sensor(Transform _offset, int _cams)
		: offset(_offset)
	{
		num_cameras = _cams;
		updated = new bool[num_cameras](); // initialized to false
		sensor_data = new Frame[num_cameras]();
	}

	// copy constructor
	Sensor(const Sensor &rhs)
		: offset(rhs.offset)
	{
		num_cameras = rhs.num_cameras;
		updated = new bool[num_cameras](); // initialized to false
		sensor_data = new Frame[num_cameras]();
	}

	// destructor
	virtual ~Sensor() {
		delete[] updated;
		delete[] sensor_data;
	}

	// offset accessor, mutator
	Transform getOffset() { 
		return offset; 
	}

	void setOffset(Transform newOffset) { 
		offset = newOffset; 
	}

	// updated accessor 
	bool getUpdated() { 
		// return true if all cameras have new data
		bool out = true;
		for (int i = 0; out && i < num_cameras; i++) {
			out = out && updated[i];
		}
		return out; 
	}

	int getNumCameras() {
		return num_cameras;
	}

	// data accessor
	virtual std::vector<Frame> Data() = 0;

	// start/stop sensor recording
	virtual int Start() = 0;
	virtual int Stop() = 0;

	// sensor name
	

protected:

	// positional offset of sensor in hololens local coordinates
	Transform offset;

	// has data been updated since last access?
	bool *updated; 

	// sensor's most current data
	Frame *sensor_data;

	// number of cameras on sensor
	int num_cameras;

};

#endif // SENSOR_H
