#include "SensorManager.h"
#include "Sensor.h"

#include <unistd.h>

int main() {

	//Transform t1;

	int num = 1;
	std::string names[num] = { "sentera" };
	SensorManager sensor_manager(num, names);
	
	//SenteraDouble4k sentera(t1);
	//usleep(3000);
	//sentera.Start();

	return 0;
}