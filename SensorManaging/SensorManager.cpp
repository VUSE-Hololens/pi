//
//
//

#include "SensorManager.h"

SensorManager::SensorManager(int _num_sensors, std::string *_sensor_names) {
	// Note: num_sensors and length of sensorNames MUST be equal
	if (num_sensors <= 0) {
		printf("Out of range error: SensorManager must be initialized with at least 1 sensor.")
		return;
	}
	num_sensors = _num_sensors;
	data_ready = false;
	sensorNames = _sensor_names;
	sensors = new Sensor[num_sensors]();

	// initialize all sensors to default offset
	Transform _offset;

	for (int i = 0; i < num_sensors; i++) {
		if (_sensor_names[i].compare("sentera") == 0) {
			SenteraDouble4k sentera(_offset);
			sensors[i] = sentera;
			sensor_keys.push_back("sentera");
			printf("Initialized sentera class \n");
		} 
		else {
			printf("Cannot initialize sensor of type %s\n", _sensor_names[i]);
			return;
		}
	}
}

SensorManager::~SensorManager() {
	StopAll();
	delete[] sensors;
}

SensorManager::checkDataReady() {
	return data_ready;
}

SensorManager::StartAll() {
	for (Sensor s : sensors) {
		if (s.Start() < 0) {
			printf("Sensor %d: failed to start\n", i);
			return -1; 
		}
	}
	return 1;
}

SensorManager::StopAll() {
	for (Sensor s : sensors) {
		if (s.Stop() < 0) {
			printf("Sensor %d: failed to stop\n", i);
			return -1;
		}
	}
	return 1;
}

SensorManager::updateImageData() {
	// check if we should process new data. customizeable to specific visualization
	bool process = true;
	for (Sensor s : sensors) {
		process = process && s.getUpdated();
	}
	if (!process) return; 

	Frame *sentera_data = sensors[0].Data();

	Vector3Int outSize(sentera_data[0].width, sentera_data[0].height, 1);
	DataProcessor::getSenteraNDVI(sentera_data, image_data.pixels, outSize);
	transform = sensors[0].getOffset();
	
}