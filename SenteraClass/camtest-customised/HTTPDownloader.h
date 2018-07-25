#pragma once
/**
* HTTPDownloader.hpp
*
* A simple C++ wrapper for the libcurl easy API.
*
* Written by Uli Köhler (techoverflow.net)
* Published under CC0 1.0 Universal (public domain)
*/
#ifndef HTTPDOWNLOADER_HPP
#define HTTPDOWNLOADER_HPP
#include <string>
#include "curl/curl.h"
#include "curl/easy.h"
#include "curl/curlbuild.h"
#include <sstream>
#include <iostream>


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
	std::string data((const char*)ptr, (size_t)size * nmemb);
	*((std::stringstream*)stream) << data << endl;
	return size * nmemb;
}

/**
* A non-threadsafe simple libcURL-easy based HTTP downloader
*/
class HTTPDownloader {
	//variables
private:
	void* curl;
	//methods
public:
	HTTPDownloader() {
		curl = curl_easy_init();
	};
	~HTTPDownloader() {
		curl_easy_cleanup(curl);
	};
	/**
	* Download a file using HTTP GET and store in in a std::string
	* @param url The URL to download
	* @return The download result
	*/
	std::string download(const std::string& url) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); //Prevent "longjmp causes uninitialized stack frame" bug
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
		std::stringstream out;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
		/* Perform the request, res will get the return code */
		CURLcode res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		}
		return out.str();
	};

private:
};
#endif  /* HTTPDOWNLOADER_HPP */