// Transmitter: Windows
// On command data tranmission
// sets up listening socket for devices to register with
// sets up transmitting socket for sending packets to all registered devices
// Configured for: WINDOWS

#pragma once
#ifndef TRANSMITTER
#define TRANSMITTER

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>

// Need to link with Ws2_32.lib
#pragma comment (lib, "ws2_32.lib")

const int MAX_PACKET_SIZE = 10000;

class Transmitter {
public:
	// constructor
	Transmitter(const char* localIP, int inPort, int outPort) {
		try {
			configure(localIP, inPort, outPort);
			start();
		}
		catch (const std::exception &exc) {
			std::cout << exc.what();
		}
	}

	// destructor
	~Transmitter() = default;

	// returns number of device registrations, NOT necessarily number of active connected devices
	int registers() {
		connectsLock.lock();
		int registers = connections.size();
		connectsLock.unlock();
		return registers;
	}

	// return most recently added registration
	sockaddr_in newestConnection() {
		return connections.back();
	}

	void clearConnections() {
		connectsLock.lock();
		connections.clear();
		connectsLock.unlock();
	}

	// transmits buffer to all connections
	void transmit(const uint8_t* buffer, const int length) {
		connectsLock.lock();
		char *bufferPtr = (char*)buffer;
		for (auto &device : connections) {
			try {
				if (sendto(outSocket, bufferPtr, length, 0, (sockaddr*)&device, sizeof(device)) == SOCKET_ERROR) {
					char errorMessage[100];
					sprintf_s(errorMessage, 100, "Send fail: %d\n", WSAGetLastError());
					throw std::runtime_error(errorMessage);
				}

				// debug
				wchar_t ipStr[16];
				InetNtop(AF_INET, &device.sin_addr, ipStr, 16);
				std::cout << "Main thread: sent message";
				std::cout << ". IP: "; std::wcout << ipStr; std::cout << ", Port: " << ntohs(device.sin_port) << "\n";
			}
			catch (std::runtime_error re) {
				std::cout << re.what();
			}
		}
		connectsLock.unlock();
	}

private:
	WSADATA wsaData;
	sockaddr_in localIn, localOut;
	SOCKET outSocket, inSocket;

	std::vector<sockaddr_in> connections;
	std::thread listeningThread;
	std::mutex connectsLock;

	// sets up local sockets
	void configure(const char* localIP, int inPort, int outPort) {
		// initialize Winsock
		int iResult;
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			char errorMessage[100];
			sprintf_s(errorMessage, 100, "WSAStartup failed: %d\n", iResult);
			throw std::runtime_error(errorMessage);
		}

		// configure local sockets
		localIn.sin_family = AF_INET;
		uint8_t localIPBinary[4]; InetPton(AF_INET, (PCWSTR)localIP, localIPBinary);
		localIn.sin_addr.s_addr = INADDR_ANY;
		localIn.sin_port = htons(inPort);

		localOut.sin_family = AF_INET;
		localOut.sin_addr.s_addr = INADDR_ANY;
		localOut.sin_port = htons(outPort);

		// create local sockets
		inSocket = INVALID_SOCKET;
		inSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (inSocket == INVALID_SOCKET) {
			char errorMessage[100];
			sprintf_s(errorMessage, 100, "Error at socket() (inSocket): %ld\n", WSAGetLastError());
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}

		outSocket = INVALID_SOCKET;
		outSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (outSocket == INVALID_SOCKET) {
			char errorMessage[100];
			sprintf_s(errorMessage, 100, "Error at socket (outSocket): %ld\n", WSAGetLastError());
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}

		// bind local sockets
		if (bind(inSocket, (struct sockaddr*)&localIn, sizeof(localIn)) == SOCKET_ERROR) {
			char errorMessage[100];
			sprintf_s(errorMessage, 100, "Error at bind (inSocket): %ld\n", WSAGetLastError());
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}
		if (bind(outSocket, (sockaddr*)&localOut, sizeof(localOut)) == SOCKET_ERROR) {
			char errorMessage[100];
			sprintf_s(errorMessage, 100, "Error at bind (outSocket): %ld\n", WSAGetLastError());
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}
	}

	// starts inSocket to listen for connections
	void start() {
		listeningThread = std::thread(&Transmitter::acceptConnections, this);
	}

	// driver of listening socket thread
	void acceptConnections() {
		sockaddr_in device;
		int deviceSize = sizeof(device);
		char inBuffer[4];
		
		while (true) {
			// recvfrom blocks thread until new data on socket
			try {
				memset(inBuffer, 0, 4);
				if (recvfrom(inSocket, inBuffer, 4, 0, (struct sockaddr *) &device, &deviceSize) == SOCKET_ERROR)
				{
					char errorMessage[100];
					sprintf_s(errorMessage, 100, "Error at recieve from (inSocket): %ld\n", WSAGetLastError());
					WSACleanup();
					throw std::runtime_error(errorMessage);
				}
			}
			catch (const std::exception &exc) {
				std::cout << exc.what();
			}

			// report any connections
			wchar_t ipStr[16];
			InetNtop(AF_INET, &device.sin_addr, ipStr, 16);
			int dataInt = *reinterpret_cast<int*>(inBuffer);

			std::cout << "Listener thread: recieved packet. Remote IP: "; std::wcout << ipStr; std::cout << ", remote port: " << ntohs(device.sin_port);
			std::cout << ". Data (as int): " << dataInt << "\n";

			// push connections with recieved ports
			device.sin_port = htons(dataInt);
			connectsLock.lock();
			connections.push_back(device);
			connectsLock.unlock();
		}
	}

	std::string bytesToHexStr(char byteArray[], int length) {
		std::stringstream ss;
		ss << std::hex;
		for (int i = 0; i < length; i++)
			ss << (int)byteArray[i];
		return ss.str();
	}
};

#endif // TRANSMITTER
