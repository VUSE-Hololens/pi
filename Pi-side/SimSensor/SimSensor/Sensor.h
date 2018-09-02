// Sensor.h
// Abstract class for sensors
// Mark Scherer, July 2018

#pragma once
#ifndef SENSOR_H
#define SENSOR_H

#include <memory>
#include <string>
#include<bitset>
#include "Transmitter.h"

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
		if (this != &rhs) {
			Frame tmp(rhs);
			std::swap(tmp, *this);
		}
		return *this;
	}

	~Frame() {
		clear();
	}

	void clear() {
		delete[] pixels;
	}

	// accessor
	uint8_t get(int i, int j, int k) {
		int index = i + j * width + k * (height * width);
		return pixels[index];
	}

	// returns size of frame once converted to buffer
	int bufferSize() {
		return height * width * bands + 20;
	}

	// converts frame to byte buffer for transmitting. Deep copies frame to buffer.
	// Conversion scheme (height*width*bands + 20 total bytes): 
		// bytes 1-12: height, width, bands (4 bytes each)
		// bytes 12-20: FOVx, FOVy (4 bytes each)
		// bytes 21+: pixels
	uint8_t* toBuffer() {
		// check for too large to send
		int length = bufferSize();
		if (length > MAX_PACKET_SIZE) {
			char errorMessage[100];
			sprintf_s(errorMessage, 100, "Packet too large to send... size: %d, MAX_PACKET_SIZE: %d\n", 
				length, MAX_PACKET_SIZE);
			throw std::runtime_error(errorMessage);
		}

		uint8_t *buffer = new uint8_t[length];

		// height, width, bands
		uint8_t *h = intToBytes(&height);
		uint8_t *w = intToBytes(&width);
		uint8_t *b = intToBytes(&bands);

		
		
		for (int i = 0; i < 4; i++) {
			buffer[i] = h[i];
		}
		for (int i = 0; i < 4; i++) {
			buffer[i + 4] = w[i];
		}
		for (int i = 0; i < 4; i++) {
			buffer[i + 8] = b[i];
		}

		// FOVx, FOVy
		uint8_t* xfov = floatToBytes(&FOVx);
		uint8_t* yfov = floatToBytes(&FOVy);
		for (int i = 0; i < 4; i++) {
			buffer[i + 12] = xfov[i];
		}
		for (int i = 0; i < 4; i++) {
			buffer[i + 16] = yfov[i];
		}

		// pixels
		for (int i = 0; i < height*width*bands; i++) {
			buffer[i + 20] = pixels[i];
		}

		return buffer;
	}

	uint8_t* intToBytes(int* value)
	{
		return static_cast<uint8_t*>(static_cast<void*>(value));
	}

	uint8_t* floatToBytes(float* value)
	{
		return static_cast<uint8_t*>(static_cast<void*>(value));
	}
};

// abstract class for all sensor inputs
class Sensor {
protected:
	const static std::string PI_IP = "192.168.143.130";

public:
	// constructor
	Sensor(Transform _offset)
		: offset(_offset)
	{
		updated = false;
		data = new Frame();
	}

	// copy constructor
	Sensor(const Sensor &rhs) 
		: offset(rhs.offset)
	{
		updated = false;
		data = new Frame();
	}

	// destructor
	virtual ~Sensor() {
		delete data;
	}

	// offset accessor, mutator
	Transform getOffset() { return offset; }
	void setOffset(Transform newOffset) { offset = newOffset; }

	// updated accessor 
	bool getUpdated() { return updated; }

	// data accessor
	virtual Frame Data() = 0;

	// start/stop sensor recording
	virtual void Start() = 0;
	virtual void Stop() = 0;

protected: 
	// positional offset of sensor in hololens local coordinates
	Transform offset;

	// has data been updated since last access?
	bool updated;

	// sensor's most current data
	Frame *data;

};

#endif // SENSOR_H
