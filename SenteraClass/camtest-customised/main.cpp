#include "SenteraDouble4k.h"
#include "Sensor.h"

#include <unistd.h>

int main() {

	Transform t1;
	SenteraDouble4k sentera(t1);

	sentera.Start();

	return 0;
}