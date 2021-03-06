#pragma once

#ifndef DATA_PROCESSOR_H_
#define DATA_PROCESSOR_H_

#include "SensorDataTypes.h"
#include <stdio.h>

class DataProcessor {

public:
	// *buf that is passed to method must be width*height in size. 
	static bool getSenteraNDVI(Frame *sensorData, int width, int height, uint8_t *buf) {

		std::size_t size = width * height * 3;

		if (sensorData[0].width != sensorData[1].width || sensorData[0].height != sensorData[1].height || sensorData[0].bands != sensorData[1].bands) {
			printf("Error: Sentera RGB and NIR Image Dimension do not match!")
			return false;
		}

		float *rgbBuf = new float[size], *nirBuf = new float[size];
		float r_rgb_tmp = 0.0f, g_rgb_tmp = 0.0f, b_rgb_tmp = 0.0f, r_nir_tmp = 0.0f, b_nir_tmp;
		float nir = 0.0f, red = 0.0f, ndvi = 0.0f;

		// loop camera
		float min_ndvi = 0;
		for (int i = 0; i < size; i += 3) {
			r_rgb_tmp = sensorData[0].pixels[i + 0];
			g_rgb_tmp = sensorData[0].pixels[i + 1];
			b_rgb_tmp = sensorData[0].pixels[i + 2];
			rgbBuf[i + 0] = +1.150 * r_rgb_tmp - 0.110 * g_rgb_tmp - 0.034 * b_rgb_tmp;
			rgbBuf[i + 1] = -0.329 * r_rgb_tmp + 1.420 * g_rgb_tmp - 0.199 * b_rgb_tmp;
			rgbBuf[i + 2] = -0.061 * r_rgb_tmp - 0.182 * g_rgb_tmp + 1.377 * b_rgb_tmp;

			// ignore green band because it does not represent any red edge or IR data
			r_nir_tmp = sensorData[1].pixels[i + 0];
			b_nir_tmp = sensorData[1].pixels[i + 2];
			nirBuf[i + 0] = +1.000 * r_nir_tmp - 0.956 * b_nir_tmp;
			nirBuf[i + 2] = -0.341 * r_nir_tmp + 2.436 * b_nir_tmp;

			nir = nirBuf[i + 2]; // blue band of NIR rgb
			red = rgbBuf[i + 0]; // red band of rgb
			ndvi = (2.700 * nir - red) / (2.700 * nir + red);

			buf[i/3] = clamp_val(255.0f*ndvi);
		}

		delete[] rgbBuf;
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
					// bilinear interpolation approximation for resizing. 
					int pixel = cz + (cx * newSize.z) + (cy * (newSize.y * newSize.z));
					int nearestMatch = (cz + ((int)(cx / scale.x) * oldSize.z) + ((int)(cy / scale.y) * (oldSize.x * oldSize.z)));
					newDataBuf[pixel] = old_data[nearestMatch];
				}
			}
		}
		return true;
	}

	static bool filterBands(Frame *sensor_data, int cam, float *nirBuf, float *rgbBuf) {
		// initialize temp values and get image size
		float r_tmp;
		float g_tmp;
		float b_tmp;
		int width = sensor_data[cam - 1].width;
		int height = sensor_data[cam - 1].height;
		int bands = sensor_data[cam - 1].bands;

		// if RGB camera
		for (int i = 0; i < width*height*bands; i += 3) {
			r_tmp = sensor_data[0].pixels[i + 0];
			g_tmp = sensor_data[0].pixels[i + 1];
			b_tmp = sensor_data[0].pixels[i + 2];
			rgbBuf[i + 0] = +1.150 * r_tmp - 0.110 * g_tmp - 0.034 * b_tmp;
			rgbBuf[i + 1] = -0.329 * r_tmp + 1.420 * g_tmp - 0.199 * b_tmp;
			rgbBuf[i + 2] = -0.061 * r_tmp - 0.182 * g_tmp + 1.377 * b_tmp;
		}

		// if red edge/NIR camera
		for (int i = 0; i < width*height*bands; i += 3) {
			// ignore green band because it does not represent any red edge or IR data
			r_tmp = sensor_data[1].pixels[i + 0];
			g_tmp = sensor_data[1].pixels[i + 1];
			b_tmp = sensor_data[1].pixels[i + 2];
			nirBuf[i + 0] = +1.000 * r_tmp - 0.956* b_tmp;
			nirBuf[i + 1] = g_tmp; // no modification here
			nirBuf[i + 2] = -0.341 * r_tmp + 2.436 * b_tmp;
		}
		return 1;
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
