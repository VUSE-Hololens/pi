#pragma once

#ifndef DATA_PROCESSOR_H_
#define DATA_PROCESSOR_H_

#include "SensorDataTypes.h"
#include <stdio.h>

class DataProcessor {

public:
	// sets buf to (R, NIR, NDVI) for each pixel in Frame
	static bool getSenteraData(Frame* sensorData, int width, int height, uint8_t *buf) {
		std::size_t size = width * height * 3;

		if (sensorData[0].width != sensorData[1].width || sensorData[0].height != sensorData[1].height || sensorData[0].bands != sensorData[1].bands) {
			printf("Error: Sentera RGB and NIR Image Dimension do not match!");
			return false;
		}

		uint32_t* r_norm = new uint32_t[height * width];
		uint32_t* nir_norm = new uint32_t[height * width];

		// separate & normalize R & NIR bands, find k
		uint8_t r, g, b, nir1, nir2;
		uint32_t r_sep, nir_sep;
		uint32_t max = 0;
		double k;
		for (int i = 0; i < width * height; i++) {
			// grab data
			r = sensorData[0].pixels[3*i + 0];
			g = sensorData[0].pixels[3*i + 1];
			b = sensorData[0].pixels[3*i + 2];
			nir1 = sensorData[1].pixels[3*i + 0];
			nir2 = sensorData[1].pixels[3*i + 2];

			// separate bands
			r_sep = 1.150*r - 0.110*g - 0.034*b;
			nir_sep = -0.341*nir1 + 2.436*nir2;

			// normalize
			r_norm[i] = r_sep / ((sensorData[0].iso / 100.0f) * (1.0f / sensorData[0].inv_ev));
			nir_norm[i] = nir_sep / ((sensorData[1].iso / 100.0f) * (1.0f / sensorData[1].inv_ev));

			// update max
			if (r_norm[i] > max) { max = r_norm[i]; }
			if (nir_norm[i] > max) { max = nir_norm[i]; }
		}

		k = 255.0 / (double)max;

		// fill in buf
		uint32_t r_pix, nir_pix;
		double ndvi;
		uint8_t r_byte, nir_byte, ndvi_byte;
		for (int i = 0; i < width * height; i++) {
			// grab data
			r_pix = r_norm[i];
			nir_pix = nir_norm[i];

			// calc NDVI
			ndvi = (2.700*nir_pix - r_pix) / (2.700*nir_pix + r_pix);

			// rescale
			r_byte = (uint8_t)((double)r_pix * k);
			nir_byte = (uint8_t)((double)nir_pix * k);
			ndvi_byte = (uint8_t)((ndvi + 1.0) * (255.0/2.0));

			// fill in buf
			buf[3*i + 0] = r_byte;
			buf[3*i + 1] = nir_byte;
			buf[3*i + 2] = ndvi_byte;
		}

		return true;
	}
	
	// currently: sets buf (pre-allocated) to array holding NDVI
	static bool getSenteraNDVI(Frame *sensorData, int width, int height, uint8_t *buf) {

		std::size_t size = width * height * 3;

		if (sensorData[0].width != sensorData[1].width || sensorData[0].height != sensorData[1].height || sensorData[0].bands != sensorData[1].bands) {
			printf("Error: Sentera RGB and NIR Image Dimension do not match!");
			return false;
		}

		// init data
		double *rgbBuf = new double[size], *nirBuf = new double[size];
		double r_rgb_tmp = 0.0f, g_rgb_tmp = 0.0f, b_rgb_tmp = 0.0f, r_nir_tmp = 0.0f, b_nir_tmp;
		double nir = 0.0f, red = 0.0f, ndvi = 0.0f;

		// loop camera
		int hist[] = { 0, 0, 0, 0};
		float min_ndvi = 0;
		for (int i = 0; i < size; i += 3) {
			// fetch raw rgb values
			r_rgb_tmp = sensorData[0].pixels[i + 0];
			g_rgb_tmp = sensorData[0].pixels[i + 1];
			b_rgb_tmp = sensorData[0].pixels[i + 2];

			// band separate rgb values
			rgbBuf[i + 0] = +1.150 * r_rgb_tmp - 0.110 * g_rgb_tmp - 0.034 * b_rgb_tmp;
			rgbBuf[i + 1] = -0.329 * r_rgb_tmp + 1.420 * g_rgb_tmp - 0.199 * b_rgb_tmp;
			rgbBuf[i + 2] = -0.061 * r_rgb_tmp - 0.182 * g_rgb_tmp + 1.377 * b_rgb_tmp;

			// grab red edge values
			// ignore green band because it does not represent any red edge or IR data
			r_nir_tmp = sensorData[1].pixels[i + 0];
			b_nir_tmp = sensorData[1].pixels[i + 2];

			// band separate red edge values
			nirBuf[i + 0] = +1.000 * r_nir_tmp - 0.956 * b_nir_tmp;
			nirBuf[i + 2] = -0.341 * r_nir_tmp + 2.436 * b_nir_tmp;

			// grab ndvi components
			nir = nirBuf[i + 2]; // blue band of NIR rgb
			red = rgbBuf[i + 0]; // red band of rgb

			// normalize images
			nir = nir / ((sensorData[1].iso / 100.0f) * (1.0f / sensorData[1].inv_ev));
			red = red / ((sensorData[0].iso / 100.0f) * (1.0f / sensorData[0].inv_ev));

			// calc ndvi
			// 2.7 scalar on ndvi accounts for camera differences
			ndvi = (2.700 * nir - red) / (2.700 * nir + red);

			//buf[i / 3] = clamp_val(255.0f*ndvi);
			buf[i / 3] = (ndvi + 1.0f) / 2.0f * 255.0f;
			/*if (buf[i / 3] <= 1 * 255 / 4) ++hist[0];
			else if (buf[i / 3] <= 2 * 255 / 4) ++hist[1];
			else if (buf[i / 3] <= 3 * 255 / 4) ++hist[2];
			else if (buf[i / 3] <= 4 * 255 / 4) ++hist[3];*/
		}
		//printf("NDVI data: <%d, %d, %d, %d>\n", hist[0], hist[1], hist[2], hist[3]);
		delete[] rgbBuf;
		delete[] nirBuf;

		return true;
	}

	// downsamples buf by dividing height and width in half
		// old_data / new_data: buffers to holding old data / to hold new data. new_data NOT initialized in method
		// oldSize: size of old buffer (width, height, bands)
		// newSize: ptr to obj to hold new size, set by this method
	static void HalfSample(uint8_t* old_data, uint8_t* new_data, Vector3Int oldSize, Vector3Int* newSize) {
		// find new size
		newSize->x = oldSize.x / 2;
		newSize->y = oldSize.y / 2;
		newSize->z = oldSize.z;

		// fill in new data
		for (int i = 0; i < newSize->x; i++) {
			for (int j = 0; j < newSize->y; j++) {
				for (int k = 0; k < newSize->z; k++) {
					uint8_t p1 = old_data[Coords2Index(2 * i, 2 * j, k, oldSize)];
					uint8_t p2 = old_data[Coords2Index(2 * i + 1, 2 * j, k, oldSize)];
					uint8_t p3 = old_data[Coords2Index(2 * i, 2 * j + 1, k, oldSize)];
					uint8_t p4 = old_data[Coords2Index(2 * i + 1, 2 * j + 1, k, oldSize)];
					new_data[Coords2Index(i, j, k, *newSize)] = (p1 + p2 + p3 + p4) / 4;
				}
			}
		}
	}

	static int Coords2Index(int i, int j, int k, Vector3Int size) {
		return j * size.z * size.x + i * size.z + k;
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
				for (int cx = 0; cx < newSize.x; cx++)
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
