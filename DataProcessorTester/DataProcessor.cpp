#include "DataProcessor.h"
#include "SensorDataTypes.h"
#include <cstdint>

int main() {
	Frame *testFrame = new Frame[2]();
	int side = 3;
	testFrame[0].pixels = new uint8_t[side*side*3];
	testFrame[1].pixels = new uint8_t[side*side*3];
	for (int i = 0; i < side; i++) {
		for (int j = 0; j < side; j++) {
			testFrame[0].pixels[0 + i * 3 + j * side*3] = 255;
			testFrame[0].pixels[0 + i * 3 + j * side*3] = 0;
			testFrame[0].pixels[0 + i * 3 + j * side*3] = 0;

			testFrame[1].pixels[0 + i * 3 + j * side*3] = 0;
			testFrame[1].pixels[0 + i * 3 + j * side*3] = 0;
			testFrame[1].pixels[0 + i * 3 + j * side*3] = 255;
		}
	}
	uint8_t *ndvibuf = new uint8_t[side*side];
	DataProcessor::getSenteraNDVI(testFrame, side, side, ndviBuf);
	printf("NDVI Array: \n");
	for (int i = 0; i < side; i++) {
		for (int j = 0; j < side; j++) {
			printf("%d ", ndvibuf[i * side*j]);
		}
		printf(";\n");
	}

}