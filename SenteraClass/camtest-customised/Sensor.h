// Sensor.h
// Abstract class for sensors
// Mark Scherer, July 2018

#pragma once
#ifndef SENSOR_H
#define SENSOR_H

#include <memory>
#include <string>

struct Vector3 {
	float x, y, z;

	// default constructor
	Vector3() {
		x = y = z = 0;
	}
};

struct Transform {
	Vector3 position, eulerAngle;
};

// single frame of recorded sensor data
struct Frame {
	uint8_t *pixels;
	int height, width, bands;
	float FOVx, FOVy;

	// default constructor
	Frame() {
		pixels = nullptr;
		height = width = bands = 1;
		FOVx = FOVy = 0;
	}

	// constructor: blank frame
	Frame(int _height, int _width, int _bands, float _FOVx, float _FOVy)
		: height(_height), width(_width), bands(_bands), FOVx(_FOVx), FOVy(_FOVy)
	{
		pixels = new uint8_t[height*width*bands];
	}

	// copy constructor: deep copy
	Frame(const Frame & rhs)
		: height(rhs.height), width(rhs.width), bands(rhs.bands), FOVx(rhs.FOVx), FOVy(rhs.FOVy)
	{
		pixels = new uint8_t[height*width*bands];
		for (int i = 0; i < height*width*bands; i++) {
			pixels[i] = rhs.pixels[i];
		}
	}

	// assignment op: deep copy
	Frame& operator=(const Frame &rhs) {
		Frame tmp(rhs);
		std::swap(tmp, *this);
	}

	~Frame() {
		clear();
	}

	// accessor
	uint8_t get(int i, int j, int k) {
		int index = i + j * width + k * (height * width);
		return pixels[index];
	}

	void clear() {
		delete[] pixels;
	}
};

// abstract class for all sensor inputs
class Sensor {
protected:
	const std::string PI_IP = "192.168.143.130";

public:
	// constructor
	Sensor(Transform _offset, int cams)
		: offset(_offset)
	{
		num_cameras = cams;
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
	Transform getOffset() { return offset; }
	void setOffset(Transform newOffset) { offset = newOffset; }

	// updated accessor 
	bool getUpdated() { return updated; }

	// data accessor
	virtual Frame Data() = 0;

	// start/stop sensor recording
	virtual int Start() = 0;
	virtual int Stop() = 0;

protected:
	// positional offset of sensor in hololens local coordinates
	Transform offset;

	// has data been updated since last access?
	bool *updated; 

	// sensor's most current data
	Frame *sensor_data;

	// number of cameras
	int num_cameras;

};

#endif // SENSOR_H
