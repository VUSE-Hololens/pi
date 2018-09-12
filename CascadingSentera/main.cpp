
#include <chrono>
#include <iomanip>

#include "SenteraDouble4k.h"

int main() {
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	fprintf(stderr, "\n\n\n\n-----------------------------------------------------------------------------------------\nNDVI Configuration: new host session begun (%s)\n\n",
		std::put_time(std::localtime((const time_t*)&now), "%F %T"));

	SenteraDouble4k sentera;
	sentera.Start();

	return 0;
}