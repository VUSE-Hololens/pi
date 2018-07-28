//
//
//

#include "SensorManager.h"

SensorManager::SensorManager() {
	// Note: num_sensors and length of sensorNames MUST be equal
	if (num_sensors <= 0) {
		printf("Out of range error: SensorManager must be initialized with at least 1 sensor.");
		return;
	}
	num_sensors = 1;
	data_ready = false;
	// initialize all sensors to default offset
	Transform _offset;
	SenteraDouble4k sentera(_offset);
	sensors = { &sentera };

}

SensorManager::~SensorManager() {
	StopAll();
	delete[] sensors;
}

bool SensorManager::checkDataReady() {
	return data_ready;
}

int SensorManager::StartAll() {
	for (Sensor s : sensors) {
		if (s.Start() < 0) {
			printf("Sensor %d: failed to start\n", i);
			return -1; 
		}
	}
	return 1;
}

int SensorManager::StopAll() {
	for (Sensor s : sensors) {
		if (s.Stop() < 0) {
			printf("Sensor %d: failed to stop\n", i);
			return -1;
		}
	}
	return 1;
}

void SensorManager::updateImageData() {
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