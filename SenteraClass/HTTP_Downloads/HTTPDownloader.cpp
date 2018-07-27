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
	std::string urlStr = "https://upload.wikimedia.org/wikipedia/commons/b/b4/JPEG_example_JPG_RIP_100.jpg";
	//std::string urlStr = "https://192.168.143.141:8080/sdcard?path=/snapshots/SenteraImagerSession1/NDRE/IMG_00001.jpg";
	std::string content = downloader.download(urlStr);
	printf("\n");
	printf("Length: %d\n", content.length());

	//unsigned char *compressedImg = content.data(); // consolidate with lines above
	strcpy((char*)compressedImg, content.c_str());

	// TO BE TESTED!!! 
	int width, height;
	int channels = 3; // i think?
	tjhandle _jpegDecompressor = tjInitDecompress();
	printf("Initialized Decompressor\n");
	tjDecompressHeader(_jpegDecompressor, compressedImg, sizeof(compressedImg), &width, &height);
	size_t size = width * height * channels;
	printf("Image Dimensions: (%d, %d, %d)\n", width, height, channels);
	unsigned char* buffer = new unsigned char[size];
	printf("Made new buffer\n");
	tjDecompress2(_jpegDecompressor, compressedImg, sizeof(compressedImg), buffer, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);
	printf("Decompressed JPG\n");
	tjDestroy(_jpegDecompressor);
	printf("Destroyed Decompressor");

}
