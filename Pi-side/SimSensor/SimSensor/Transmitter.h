// Transmitter
// On command data tranmission
// sets up listening socket for devices to register with
// sets up transmitting socket for sending packets to all registered devices
// Configured for: WINDOWS

#pragma once
#ifndef TRANSMITTER
#define TRANSMITTER

#include<WinSock2.h>
#include<iostream>
#include<vector>
#include<thread>

class Transmitter {
	const int PACKET_SIZE = 10000;
public:
	// constructor
	Transmitter(const char* localIP, int inPort, int outPort) {
		configure(localIP, inPort, outPort);
		start();
	}

	// destructor
	~Transmitter() = default;

	// returns number of device registrations, NOT necessarily number of active connected devices
	int registers() {
		return connections.size();
	}

	void clearConnections() {
		connections.clear();
	}

	// transmits buffer to all connections
	void transmit(const uint8_t* buffer, const int length) {
		for (int i = 0; i < connections.size; i++) {
			sendto(outSocket, (char*)buffer, length, 0, &connections[i], sizeof(connections[i]));
		}
	}

private:
	WSADATA wsaData;
	sockaddr_in localIn, localOut;
	SOCKET outSocket, inSocket;
	std::vector<sockaddr> connections;
	std::thread listeningThread;

	// sets up local sockets
	void configure(const char* localIP, int inPort, int outPort) {
		// initialize Winsock
		int iResult;
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			char errorMessage[100];
			sprintf(errorMessage, "WSAStartup failed: %d\n", iResult);
			throw std::runtime_error(errorMessage);
		}

		// configure local sockets
		localIn.sin_family = AF_INET;
		localIn.sin_addr.s_addr = inet_addr(localIP);
		localIn.sin_port = inPort;

		localOut.sin_family = AF_INET;
		localOut.sin_addr.s_addr = inet_addr(localIP);
		localOut.sin_port = outPort;

		// create, bind local sockets
		inSocket = INVALID_SOCKET;
		inSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (inSocket == INVALID_SOCKET) {
			char errorMessage[100];
			sprintf(errorMessage, "Error at socket() (inSocket): %ld\n", WSAGetLastError());
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}
		bind(inSocket, (sockaddr*)&localIn, sizeof(localIn));

		outSocket = INVALID_SOCKET;
		outSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (outSocket == INVALID_SOCKET) {
			char errorMessage[100];
			sprintf(errorMessage, "Error at socket() (outSocket): %ld\n", WSAGetLastError());
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}
		bind(inSocket, (sockaddr*)&localOut, sizeof(localOut));
	}

	// starts inSocket to listen for connections
	void start() {
		if (listen(inSocket, SOMAXCONN) == SOCKET_ERROR) {
			char errorMessage[100];
			sprintf(errorMessage, "Listen failed with error: %ld\n", WSAGetLastError());
			closesocket(inSocket);
			WSACleanup();
			throw std::runtime_error(errorMessage);
		}

		std::thread listeningThread(&acceptConnections);
	}

	// driver of listening socket thread
	void acceptConnections() {
		sockaddr device[1];
		int devicesLength = 1;
		char dummy[1];
		
		while (true) {
			// recvfrom blocks thread until new data on socket
			recvfrom(inSocket, dummy, 1, 0, device, &devicesLength);
			connections.push_back(device[1]);
		}
	}
};

#endif // TRANSMITTER
