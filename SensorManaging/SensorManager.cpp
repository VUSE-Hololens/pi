//
//
//

#include "SensorManager.h"

SensorManager::SensorManager() {
	data_ready = false;
	// initialize all sensors to default offset
	Transform _offset;
	sentera = new SenteraDouble4k(_offset);
}

SensorManager::~SensorManager() {
	delete sentera;
}

bool SensorManager::checkDataReady() {
	return data_ready;
}

void SensorManager::updateImageData() {
	// check if we should process new data. customizeable to specific visualization
	
	if (!sentera->getUpdated()) return; 

	int cams = sentera->getNumCameras();
	Frame sentera_data = sentera->Data();

	//Vector3Int outSize(sentera_data[0].width, sentera_data[0].height, 1);
	//DataProcessor::getSenteraNDVI(sentera_data, image_data.pixels, outSize);
	//transform = sensors[0].getOffset();
	
}