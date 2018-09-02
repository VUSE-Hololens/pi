// Serializer.h
// Static class for serializing and de-serializing data for transmission

// NOTE: byte type represented as uint8_t 

// must pass uint8_t array of sufficient size 'dest' to be filled with output

#pragma once
#ifndef SERIALIZER
#define SERIALIZER

#include <cstdint>
#include <cstring>
#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class Serializer {
public:
	// int (4 bytes)
	static void serializeInt(uint8_t* dest, int data) {
		memcpy(dest, &data, 4);
	}
	static void deserializeInt(int* dest, uint8_t* data) {
		memcpy(dest, data, 4);
	}

	// IP address (4 bytes)
	// Note: required initialized WSA
	static void serializeIP(uint8_t* dest, std::string ip) {
		int resultCode = inet_pton(AF_INET, ip, dest);
		if (resultCode != 1) {
			std::cout << "IP string to binary conversion failed with error code " << errno << "\n";
		}
	}
	static void deserializeIP(std::string* dest, uint8_t* data) {
		char _dest[INET_ADDRSTRLEN];
		int resultCode = inet_ntop(AF_INET, data, _dest, INET_ADDRSTRLEN);
		if (resultCode != 1) {
			std::cout << "IP binary to wstring conversion failed with error code " << WSAGetLastError() << "\n";
		}
		*dest = std::string(_dest);
	}
};

#endif // SERIALIZER
