
#include <chrono>
#include <iomanip>

#include "SenteraDouble4k.h"

int main() {
	// get time 
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
	std::string str(buffer);

	fprintf(stderr, "\n\n\n\n-----------------------------------------------------------------------------------------\nNDVI Configuration: new host session begun (%s)\n\n",
		str);

	SenteraDouble4k sentera;
	sentera.Start();

	return 0;
}