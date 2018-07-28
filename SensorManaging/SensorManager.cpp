//
//
//

#include "SensorManager.h"

SensorManager::SensorManager() {
	data_ready = false;
	// initialize all sensors to default offset
	Transform _offset;
	//sentera = SenteraDouble4k(_offset);
	live_session = false;
}

SensorManager::~SensorManager() {
	//delete sentera;
}

bool SensorManager::checkDataReady() {
	return data_ready;
}

void SensorManager::StartSession() {

	std::thread t1(sentera.Start);

	live_session = true;
	//std::chrono::milliseconds dura(100);

	while (live_session) {
		printf("Live Session\n");
		if (!(sentera.getUpdated())) {
			//std::this_thread::sleep_for(dura);
			continue;
		}
		updateImageData();
	}
}

void SensorManager::updateImageData() {
	// check if we should process new data. customizeable to specific visualization

	int cams = sentera.getNumCameras();
	printf("Sentera has %d cams\n", cams);
	std::vector<Frame> sentera_data = sentera.Data();
	printf("Sentera FOV = (%0.2f, %0.2f)", sentera_data.at(0).width, sentera_data.at(0).height);

	//Vector3Int outSize(sentera_data[0].width, sentera_data[0].height, 1);
	//DataProcessor::getSenteraNDVI(sentera_data, image_data.pixels, outSize);
	//transform = sensors[0].getOffset();
}