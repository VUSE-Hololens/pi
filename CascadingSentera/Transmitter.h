// Transmitter.h
// Class for dual communication with client (Both UDP). Primary for sending data, secondary for two way comm
// configuration: LINUX

#ifndef TRANSMITTER
#define TRANSMITTER

// networking dependencies
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// other dependencies
#include <vector>
#include <algorithm>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <istream>

// dependent classes
#include "Serializer.h"

class Transmitter {
public:
	// transmission paramters
	const int MAX_PACKET_SIZE = 35000; // bytes, all inclusive max size of packets
	const int HEADER_SIZE = 12; // bytes, size of header of int32 of messages on primary socket

	// command codes
	const int CONNECT = 1;
	const int DISCONNECT = 2;

	// misc constants
	const int SOCKET_ERROR = -1;
	const int WOULD_BLOCK = 11;


	// constructor
	Transmitter(std::string _localIP, int _primPort, int _secPort)
		: localIP(_localIP), primPort(_primPort), secPort(_secPort)
	{
		// create sockets
		primSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (primSocket == SOCKET_ERROR) {
			fprintf(stderr, "Primary socket construction failed with error code: %d\n", errno);
			return;
		}
		else {
			fprintf(stderr, "Successfully constructed primary socket\n");
		}
		secSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (secSocket == SOCKET_ERROR) {
			fprintf(stderr, "Secondar socket construction failed with error code: %d\n", errno);
			return;
		}
		else {
			fprintf(stderr, "Successfully constructed secondary socket\n");
		}

		// bind  UDP Socket to local port
		sockaddr_in primSocketAddr = createSockAddr(localIP, primPort);

		// test binding to all interfaces
		primSocketAddr.sin_addr.s_addr = INADDR_ANY;

		int resultCode = bind(primSocket, (const sockaddr*)&primSocketAddr, (socklen_t)sizeof(primSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			fprintf(stderr, "Primary socket binding failed with error code: %d\n", errno);
		}
		else {
			fprintf(stderr, "Successfully bound primary socket to %d-%d", localIP, primPort);
		}
		sockaddr_in secSocketAddr = createSockAddr(localIP, secPort);
		
		// test binding to all interfaces
		secSocketAddr.sin_addr.s_addr = INADDR_ANY;

		resultCode = bind(secSocket, (const sockaddr*)&secSocketAddr, (socklen_t)sizeof(secSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			fprintf(stderr, "Primary socket binding failed with error code: %d\n", errno);
		}
		else {
			fprintf(stderr, "Successfully bound secondary socket to %d-%d", localIP, secPort);
		}

		// start listening on secSocket
		connected = false;
		listening = true;
		listeningThread = std::thread(std::bind(&Transmitter::listen, this));
	}

	// destructor
	~Transmitter() {
		listening = false;
		listeningThread.join();

		close(primSocket);
		close(secSocket);
	}

	// connect
	void connectDevice(sockaddr_in _primConn, sockaddr_in _secConn) {
		lock.lock();

		primConn = _primConn;
		secConn = _secConn;

		// connect local outSocket
		fprintf(stderr, "Connecting device (primary socket): %s\n", SockAddrToStr(primConn));
		connected.store(true);
		// NOTE: connect() unnecessary as transmit() uses sendto with primConn...
		int resultCode = connect(primSocket, (sockaddr*)&primConn, sizeof(primConn));
		if (resultCode == SOCKET_ERROR) {
			fprintf(stderr, "Failed to connect primary socket with error code: %d\n", errno);
			connected.store(false);
		}
		else {
			fprintf(stderr, "Succesfully connected to host: %s / %s", SockAddrToStr(primConn), SockAddrToStr(secConn));
		}

		lock.unlock();
	}

	// disconnect
	void disconnectDevice() {
		connected.store(false);
	}

	// hasConnection
	bool hasConnection() {
		return connected.load();
	}

	// connectionsDetails
	std::string connectionDetails() {
		if (connected.load()) {
			return SockAddrToStr(primConn) + " / " + SockAddrToStr(secConn);
		}
		else {
			return "Not connected.";
		}
	}

	// transmit
	void transmit(char* data, int length) {
		if (length > MAX_PACKET_SIZE) {
			fprintf(stderr, "Data too large to send. Max packet size: %d, length: %d\n", MAX_PACKET_SIZE, length);
			return;
		}

		if (!connected.load()) {
			fprintf(stderr, "Tried to transmit but no connected device.\n");
			return;
		}

		lock.lock();

		int resultCode = sendto(primSocket, data, length, 0, (sockaddr*)&primConn, sizeof(primConn));
		if (resultCode == SOCKET_ERROR) {
			fprintf(stderr, "sendto failed with error: %d\n", errno);
		}

		lock.unlock();

		fprintf(stderr, "Successfully sent transmission to: %s\n", SockAddrToStr(primConn));
	}

private:
	// data
	int primSocket, secSocket;
	sockaddr_in primConn, secConn;
	std::string localIP;
	int primPort, secPort;

	std::atomic<bool> listening;
	std::atomic<bool> connected;
	std::mutex lock;
	std::thread listeningThread;

	// listen
	void listen() {
		char recvBuf[MAX_PACKET_SIZE];
		int bufLen = MAX_PACKET_SIZE;
		sockaddr_in sender;
		int senderSize = sizeof(sender);

		// debug
		fprintf(stderr, "Starting listening (blocking) on secondary socket on dedicated thread...\n");

		while (listening.load()) {
			// receive datagram
			int resultCode = recvfrom(secSocket, recvBuf, bufLen, 0, (sockaddr*)&sender, (socklen_t*)&senderSize);
			if (resultCode == SOCKET_ERROR) {
				fprintf(stderr, "Secondary socket receiving failed with error code: %d\n", errno);
			}
			else {
				// debug
				fprintf(stderr, "Received message on secondary socket: %s, from: %s - %d\n",
					recvBuf, SockAddrToStr(sender).c_str(), sender.sin_port);

				// check sender validity
				if (connected.load() && SockAddrToStr(sender) != SockAddrToStr(secConn)) {
					fprintf(stderr, "Received packet not originating from connected device. Connected to (secondary socket): %s, received from: %s\n",
						SockAddrToStr(secConn).c_str(), SockAddrToStr(secConn).c_str());
				}
				else {
					handlePacket(recvBuf, sender);
				}
			}
		}

		fprintf(stderr, "Listening stopped...\n");
	}

	// handlePacket
	void handlePacket(char recvBuf[], sockaddr_in sender) {
		// check command
		int command;
		Serializer::deserializeInt(&command, (uint8_t*)recvBuf);

		// debug
		fprintf(stderr, "Received packet\n");

		if (command == CONNECT) {
			// parse request details
			int port; Serializer::deserializeInt(&port, (uint8_t*)recvBuf + 4);
			sockaddr_in newSecConn = createSockAddr(sender.sin_addr, sender.sin_port);
			sockaddr_in newPrimConn = createSockAddr(sender.sin_addr, htons(port));

			fprintf(stderr, "Received connection request: %s / %s\n", SockAddrToStr(newPrimConn), SockAddrToStr(newSecConn));

			// connect to device
			connectDevice(newPrimConn, newSecConn);
		}
		else if (command == DISCONNECT) {
			disconnectDevice();
		}
		else {
			fprintf(stderr, "Received unrecognized command: %d\n", command);
		}
	}

	// helpers

	// createSockAddr
	sockaddr_in createSockAddr(in_addr ip, int port) {
		sockaddr_in result;
		result.sin_family = AF_INET;
		result.sin_addr = ip;
		result.sin_port = port;
		return result;
	}
	// createSockAddr
	sockaddr_in createSockAddr(std::string ip, int port) {
		sockaddr_in result;
		result.sin_family = AF_INET;
		result.sin_addr.s_addr = inet_addr(ip.c_str());
		result.sin_port = htons(port);
		return result;
	}

	// SockAddrToStr
	std::string SockAddrToStr(sockaddr_in sa) {
		char result[50];
		// convert IP to string
		char str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);
		// format return
		sprintf(result, "%s-%d", str, ntohs(sa.sin_port));
		return result;
	}
};

#endif // TRANSMITTER
