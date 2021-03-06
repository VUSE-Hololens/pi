#pragma once

#ifndef DATA_PROCESSOR_H_
#define DATA_PROCESSOR_H_

#include "SensorDataTypes.h"

#include <cstdio>
#include <cstdint>

class DataProcessor {

public:
	// *buf that is passed to method must be width*height in size. 
	// params:
	// sensorData: array of Frames holding sensor's data. Index 0's pixels assigned to rgbBuf, Index1's to nirBuf.
	// width, height: used to specify dimensions of output pixel array
	// buf: ptr to output pixel array (actually 1D buffer)
	static bool getSenteraNDVI(Frame *sensorData, int width, int height, uint8_t *buf) {
		// verify destination array is null
		if (!buf) {
			printf("Error: passed buffer pointer for output must be null\n");
			return false;
		}

		// create tmp buffers for resized input buffers
		uint8_t *nirBuf, *rgbBuf;

		// check input, destination sizes
		Vector3Int newSize(width, height, 3);
		//DEBUG printf("New Size <%d, %d, %d>\n", newSize.x, newSize.y, newSize.z);
		Vector3Int rgbSize(sensorData[0].width, sensorData[0].height, sensorData[0].bands);
		//DEBUG printf("RGB Img Size <%d, %d, %d>\n", rgbSize.x, rgbSize.y, rgbSize.z);
		Vector3Int nirSize(sensorData[1].width, sensorData[1].height, sensorData[1].bands);
		//DEBUG printf("NIR Img Size <%d, %d, %d>\n", nirSize.x, nirSize.y, nirSize.z);

		// resize input buffers
		if (!newSize.equals(rgbSize)) {
			printf("rgbSize != newSize\n");
			uint8_t *rgbBuf = new uint8_t[newSize.x * newSize.y * newSize.z];
			Resample(sensorData[0].pixels, rgbSize, newSize, rgbBuf);
		}
		else {
			rgbBuf = sensorData[0].pixels;
		}
		if (!newSize.equals(nirSize)) {
			printf("nirSize != newSize\n");
			Resample(sensorData[1].pixels, nirSize, newSize, nirBuf);
		}
		else {
			nirBuf = sensorData[1].pixels;
		}

		//DEBUG printf("nirBuf and rgbBuf initialized\n");


		// pre-loop declarations, fresh metadata
		uint8_t nir;
		uint8_t red;
		float ndvi = 0.0f;
		int negCount = 0, inBounds = 0, abvCount = 0;

		// cycle thru destination buffer
		for (int i = 0; i < newSize.x; i++)
		{
			for (int j = 0; j < newSize.y; j++)
			{
				// grab calculation inputs from input buffers
				nir = nirBuf[2 + (i*newSize.z) + (j*newSize.z*newSize.y)]; // blue band of NIR rgb
				red = rgbBuf[0 + (i*newSize.z) + (j*newSize.z*newSize.y)]; // red band of rgb

																		   // calculate, set NDVI in destination buffer
				ndvi = (2.700 * nir - red) / (2.700 * nir + red);
				buf[i + newSize.x * j] = clamp_val(ndvi);

				// update metadata
				if (ndvi < 0) {
					negCount++;
				}
				else if (ndvi < 256) {
					inBounds++;
				}
				else {
					abvCount++;
				}
			}
		}
		printf("Count: <%d, %d, %d>: total = %d = %d\n", negCount, inBounds, abvCount, negCount + inBounds + abvCount, newSize.x * newSize.y);

		// de-allocate temporary resized input buffers in req'd allocation
		if (!newSize.equals(rgbSize)) delete[] rgbBuf;
		if (!newSize.equals(nirSize)) delete[] nirBuf;
		//DEBUG printf("NDVI calculated. Exiting getSenteraNDVI.\n");
		return true;
	}

	// *buf that is passed to method must be width*height in size. 
	static bool getSenteraNDRE(Frame *sensorData, int width, int height, uint8_t *buf) {
		Vector3Int newSize(width, height, 3);
		Vector3Int nirSize(sensorData[1].width, sensorData[1].height, sensorData[1].bands); // only need NIR cam data

		uint8_t  *nirBuf;
		if (!newSize.equals(nirSize)) {
			nirBuf = new uint8_t[newSize.x * newSize.y * newSize.z];
			Resample(sensorData[1].pixels, nirSize, newSize, nirBuf);
		}
		else {
			nirBuf = sensorData[1].pixels;
		}

		uint8_t nir;
		uint8_t red_edge;
		float ndre;
		for (int i = 0; i < newSize.x; i++)
		{
			for (int j = 0; j < newSize.y; j++)
			{
				nir = nirBuf[2 + (i*newSize.z) + (j*newSize.z*newSize.y)]; // blue band of NIR rgb
				red_edge = nirBuf[0 + (i*newSize.z) + (j*newSize.z*newSize.y)]; // red edge band of rgb
				ndre = (1.0*nir - red_edge) / (1.0*nir + red_edge);
				buf[i + newSize.x * j] = clamp_val(ndre);
			}
		}
		if (!newSize.equals(nirSize)) delete[] nirBuf;
		return true;
	}

	static bool Resample(uint8_t *old_data, Vector3Int oldSize, Vector3Int newSize, uint8_t *newDataBuf)
	{
		if (old_data == NULL) return false;
		//
		// Get a new buffer to interpolate into
		Vector3 scale((float)newSize.x / (float)oldSize.x, (float)newSize.y / (float)oldSize.y, (float)newSize.z / (float)oldSize.z);

		for (int cz = 0; cz < newSize.z; cz++)
		{
			for (int cy = 0; cy < newSize.y; cy++)
			{
				for (int cx = 0; cx < newSize.z; cx++)
				{
					// bilinear interpolation approximation for resizing. 
					int pixel = cz + (cx * newSize.z) + (cy * (newSize.y * newSize.z));
					int nearestMatch = (cz + ((int)(cx / scale.x) * oldSize.z) + ((int)(cy / scale.y) * (oldSize.x * oldSize.z)));
					newDataBuf[pixel] = old_data[nearestMatch];
				}
			}
		}
		return true;
	}

private:
	// clamp float to uint8_t
	static uint8_t clamp_val(float n) {
		if (n < 0) return (uint8_t)0;
		if (n > 255) return (uint8_t)255;
		return (uint8_t)((int)n & 0xff);
	}
};

#endif // !DATA_PROCESSOR_H_