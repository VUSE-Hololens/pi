#pragma once

#ifndef DATA_PROCESSOR_H_
#define DATA_PROCESSOR_H_

#include <string>

#include "SensorDataTypes.h"

class DataProcessor {

public:
	static bool getSenteraNDVI(Frame *sensorData, uint8_t *buf, Vector3Int _newSize) {
		Vector3Int newSize(_newSize.x, _newSize.y, 3);
		Vector3Int rgbSize(sensorData[0].width, sensorData[0].height, sensorData[0].bands);
		Vector3Int nirSize(sensorData[1].width, sensorData[1].height, sensorData[1].bands);

		uint8_t *rgbBuf, *nirBuf;
		if (!newSize.equals(rgbSize)) {
			rgbBuf = new uint8_t[newSize.x * newSize.y * newSize.z];
			Resample(sensorData[0].pixels, rgbSize, newSize, rgbBuf);
		}
		else {
			rgbBuf = sensorData[0].pixels;
		}
		if (!newSize.equals(nirSize)) {
			nirBuf = new uint8_t[newSize.x * newSize.y * newSize.z];
			Resample(sensorData[1].pixels, nirSize, newSize, nirBuf)
		}
		else {
			nirBuf = sensorData[1].pixels;
		}

		delete[] buf;
		buf = new uint8_t[newSize.x * newSize.y];

		uint8_t nir;
		uint8_t red; 
		float ndvi;
		for (int i = 0; i < newSize.x; i++) 
		{
			for (int j = 0; j < newSize.y; j++)
			{
				nir = nirBuf[2 + (i*newSize.z) + (j*newSize.z*newSize.y)]; // blue band of NIR rgb
				red = rgbBuf[0 + (i*newSize.z) + (j*newSize.z*newSize.y)]; // red band of rgb
				ndvi = (2.700 * nir - red) / (2.700 * nir + red);
				buf[i + newSize.x * j] = clamp_val(ndvi);
			}
		}
		delete[] rgbBuf;
		delete[] nirBuf;
		return true;
	}

	static bool getSenteraNDRE(Frame *sensorData, uint8_t *buf, Vector3Int newSize) {
		Vector3Int nirSize(sensorData[1].width, sensorData[1].height, sensorData[1].bands); // only need NIR cam data

		uint8_t  *nirBuf;
		if (!newSize.equals(nirSize)) {
			nirBuf = new uint8_t[newSize.x * newSize.y * newSize.z];
			Resample(sensorData[1].pixels, nirSize, newSize, nirBuf);
		}
		else {
			nirBuf = sensorData[1].pixels;
		}

		// reinitialize buffer to appropriate length
		delete[] buf;
		buf = new uint8_t[newSize.x * newSize.y];
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
				buf[i + newSize.x * j] = this::clamp_val(ndre);
			}
		}
		delete[] nirBuf;
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
					int pixel = cz + (cx * newSize.z) + (cy * (newSize.y * newSize.z));
					int nearestMatch = (cz + ((int)(cx / scale.x) * oldSize.z) + ((int)(cy / scale.y) * (oldSize.x * oldSize.z)));
					newDataBuf[pixel] = old_data[nearestMatch];
				}
			}
		}
		return true;
	}

	//TODO: getTransform (get cumulative image transform)

private:
	// clamp float to uint8_t
	static uint8_t clamp_val(float n) {
		if (n < 0) return (uint8_t)0;
		if (n > 255) return (uint8_t)255;
		return (uint8_t)n;
	}
};

#endif // !DATA_PROCESSOR_H_
