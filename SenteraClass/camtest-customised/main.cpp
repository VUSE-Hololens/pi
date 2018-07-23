#include "SenteraDouble4k.h"
#include <unistd.h>

int main() {
	SenteraDouble4k sentera;

	usleep(5000);

	sentera.Start();

	return 0;
}