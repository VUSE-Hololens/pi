#pragma once

//#include "opencv2/opencv.hpp"
#include "turbojpeg.h"
#include "cstdint"
#include <vector>

class compress
{
public:
	compress();
	~compress();


	long unsigned int compressRGBJpeg(uint8_t* frame, uint8_t** compressed, int width, int height, int quality);
	long unsigned int compressBandJpeg(uint8_t* frame, uint8_t **compressed, int width, int height, int quality);

	//cv::Mat* getMatFromArray(uint8_t*** ar, int rows, int cols, int bands);
	//uint8_t* getArrayFromMat(cv::Mat frame);
private:

	std::vector<int> params = std::vector<int>(2);

	tjhandle jpegCompressor;
};

