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

int main(int argc, char** argv) {
	HTTPDownloader downloader;
	std::string urlStr = "https://upload.wikimedia.org/wikipedia/commons/b/b4/JPEG_example_JPG_RIP_100.jpg";
	std::string content = downloader.download(urlStr);
	std::cout << content << std::endl;
}