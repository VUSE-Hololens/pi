#pragma once

#ifndef SENSOR_MANAGER_H_
#define SENSOR_MANAGER_H_

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
	SenteraDouble4k sentera;

private:
	// frame of one band data to be compressed and transmitted
	Frame image_data;

	// transform of processed image
	Transform transform;

	// number of sensors
	int num_sensors;

	// is data ready to be transmitted
	bool data_ready;

	bool live_session;

	// methods
public:
	SensorManager();
	~SensorManager();

	// check if data ready to be sent
	bool checkDataReady();

	// start session
	void StartSession();

private:
	void updateImageData();

};

#endif // !SENSOR_MANAGER_H_
