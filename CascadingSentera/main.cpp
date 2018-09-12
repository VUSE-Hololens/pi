
#include <chrono>

#include "SenteraDouble4k.h"

int main() {

	fprintf(stderr, "\n\n\n\n-----------------------------------------------------------------------------------------\n" + 
		"NDVI Configuration: new host session begun (%s)\n\n",
		std::put_time(std::localtime(&std::chrono::system_clock::now()), "%F %T"));

	SenteraDouble4k sentera;
	sentera.Start();

	return 0;
}