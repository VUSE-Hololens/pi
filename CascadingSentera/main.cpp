
#include <chrono>
#include <iomanip>

#include "SenteraDouble4k.h"

int main() {
	// get time 
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::tm* localtime = std::localtime((const time_t*)&now);
	char timeBuf[100];
	strftime(timeBuf, 100, "%d-%m-%Y %H:%M:%S", localtime);

	fprintf(stderr, "\n\n\n\n-----------------------------------------------------------------------------------------\nNDVI Configuration: new host session begun (%s)\n\n",
		timeBuf);

	SenteraDouble4k sentera;
	sentera.Start();

	return 0;
}