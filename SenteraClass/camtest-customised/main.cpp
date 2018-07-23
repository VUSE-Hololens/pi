#include "SenteraDouble4k.h"
#include "Sensor.h"

#include <unistd.h>

int main() {

	Transform t1;
	SenteraDouble4k sentera(t1);
	usleep(3000);
	sentera.Start();

	return 0;
}