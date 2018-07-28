#pragma once

#ifndef SENSOR_MANAGER_H_
#define SENSOR_MANAGER_H_

#include <map>
#include <vector>
#include <string>

// user includes
#include "SensorDataTypes.h"
#include "Sensor.h"
#include "DataProcessor.h"
#include "SenteraDouble4k.h"
#include "DataProcessor.h"

class SensorManager {

	// variables
public:
	// array of sensors to be managed
	Sensor *sentera;

private:
	// frame of one band data to be compressed and transmitted
	Frame image_data;

	// transform of processed image
	Transform transform;

	// number of sensors
	int num_sensors;

	// is data ready to be transmitted
	bool data_ready;

	// methods
public:
	SensorManager();
	~SensorManager();

	// check if data ready to be sent
	bool checkDataReady();

private:
	void updateImageData();

};

#endif // !SENSOR_MANAGER_H_
