/**
* HTTPDownloaderExample.cpp
*
* A simple C++ wrapper for the libcurl easy API.
* This file contains example code on how to use the HTTPDownloader class.
*
* Compile like this: g++ -o HTTPDownloaderExample HTTPDownloaderExample.cpp HTTPDownloader.cpp -lcurl
*
* Written by Uli Köhler (techoverflow.net)
* Published under CC0 1.0 Universal (public domain)
*/
//#include "stdafx.h"
#include "HTTPDownloader.h"
#include <turbojpeg.h>

int main(int argc, char** argv) {
	HTTPDownloader downloader;
	std::string urlStr = "http://www.simpopdf.com/sample/image-to-pdf-sample.jpg";

	std::string content = downloader.download(urlStr);
	int imgLength = content.length();
	printf("Length: %d\n", imgLength);

	// consolidate with above tbd
	unsigned char* compressedImg = (unsigned char*)content.c_str();

	// TO BE TESTED!!! 
	int width, height;
	int channels = 3; // i think?
	tjhandle _jpegDecompressor = tjInitDecompress();
	printf("Initialized Decompressor\n");
	printf("ImgSize: %d\n", imgLength);
	tjDecompressHeader(_jpegDecompressor, compressedImg, imgLength, &width, &height);
	size_t size = width * height * channels;
	printf("Image Dimensions: (%d, %d, %d)\n", width, height, channels);
	unsigned char* buffer = new unsigned char[size];
	printf("Made new buffer\n");
	tjDecompress2(_jpegDecompressor, compressedImg, imgLength, buffer, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);
	printf("Decompressed JPG\n");
	tjDestroy(_jpegDecompressor);
	printf("Destroyed Decompressor");

}
