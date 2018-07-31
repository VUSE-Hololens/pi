#pragma once

#include <cstdint>

// vector 3
struct Vector3 {
	float x, y, z;

	// default constructor
	Vector3() {
		x = y = z = 0;
	}

	Vector3(float _x, float _y, float _z) {
		x = _x; 
		y = _y;
		z = _z;
	}

	bool equals(Vector3 & rhs) {
		return (x == rhs.x && y == rhs.y && z == rhs.z);
	}
};

struct Vector3Int {
	int x, y, z;

	// default constructor
	Vector3Int() {
		x = y = z = 0;
	}

	Vector3Int(int _x, int _y, int _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	bool equals(Vector3Int & rhs) {
		return (x == rhs.x && y == rhs.y && z == rhs.z);
	}
};

// positional transform
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