#include "SenteraDouble4k.h"
#include <unistd.h>

int main() {

	Transform t1;
	SenteraDouble4k sentera(t1);

	usleep(5000);

	sentera.Start();

	return 0;
}