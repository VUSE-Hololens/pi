#pragma once

#include "opencv2/opencv.hpp"
#include "turbojpeg.h"

class compress
{
public:
	compress();
	~compress();


	long unsigned int compressRGBJpeg(uchar* frame, uchar** compressed, int width, int height, int quality);
	long unsigned int compressBandJpeg(uchar* frame, uchar **compressed, int width, int height, int quality);

	cv::Mat* getMatFromArray(uchar*** ar, int rows, int cols, int bands);
	uchar* getArrayFromMat(cv::Mat frame);
private:

	std::vector<int> params = std::vector<int>(2);

	tjhandle jpegCompressor;
};

