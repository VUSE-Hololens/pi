
#include <chrono>
#include <iomanip>

#include "SenteraDouble4k.h"

int main() {
	auto now = std::chrono::system_clock::now();

	fprintf(stderr, "\n\n\n\n-----------------------------------------------------------------------------------------\nNDVI Configuration: new host session begun (%s)\n\n",
		std::put_time(std::localtime(&now), "%F %T"));

	SenteraDouble4k sentera;
	sentera.Start();

	return 0;
}