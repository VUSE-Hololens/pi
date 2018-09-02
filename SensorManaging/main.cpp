
#include <unistd.h>
#include <string>
#include <chrono>

#include "SensorManager.h"
#include "Sensor.h"

int main() {

	//Transform t1;

	//int num = 1;
	//std::string names[num] = { "sentera" };
	//SensorManager sensor_manager;

	//std::chrono::seconds waittime(3);
	//std::this_thread::sleep_for(waittime);

	//sensor_manager.StartSession();
	
	SenteraDouble4k sentera;
	sentera.Start();

	return 0;
}