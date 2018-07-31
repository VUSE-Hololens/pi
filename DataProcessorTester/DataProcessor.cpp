
#include <cstdint>
#include <stdio.h>

#include "DataProcessor.h"
#include "SensorDataTypes.h"

int main() {
	Frame *testFrame = new Frame[2]();
	int width = 4, height = 3;
	testFrame[0].pixels = new uint8_t[width*height*3];
	testFrame[1].pixels = new uint8_t[width*height*3];
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			testFrame[0].pixels[0 + i * 3 + j * width*3] = 255;
			testFrame[0].pixels[1 + i * 3 + j * width*3] = 0;
			testFrame[0].pixels[2 + i * 3 + j * width*3] = 0;

			testFrame[1].pixels[0 + i * 3 + j * width*3] = 0;
			testFrame[1].pixels[1 + i * 3 + j * width*3] = 0;
			testFrame[1].pixels[2 + i * 3 + j * width*3] = 255;
		}
	}
	uint8_t *ndvibuf = new uint8_t[width*height];
	DataProcessor::getSenteraNDVI(testFrame, width, height, ndvibuf);
	printf("NDVI Array: \n");
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			printf("%d ", ndvibuf[i * width*j]);
		}
		printf(";\n");
	}
	//testFrame[0].clear();
	//testFrame[1].clear();
	delete[] ndvibuf;
	delete[] testFrame;
	printf("-------------\n");

}