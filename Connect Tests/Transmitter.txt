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
	const int MAX_PACKET_SIZE = 10000; // bytes, all inclusive max size of packets
	const int HEADER_SIZE_PRIM = 8; // bytes, size of header of int32 of messages on primary socket

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
			std::cout << "Primary socket construction failed with error code: " << errno << "\n";
			return;
		}
		else {
			std::cout << "Successfully constructed primary socket\n";
		}
		secSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (secSocket == SOCKET_ERROR) {
			std::cout << "Secondary socket construction failed with error code: " << errno << "\n";
			return;
		}
		else {
			std::cout << "Successfully constructed secondary socket\n";
		}

		// bind  UDP Socket to local port
		sockaddr_in primSocketAddr = createSockAddr(localIP, primPort);
		int resultCode = bind(primSocket, (const sockaddr*)&primSocketAddr, (socklen_t)sizeof(primSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "Primary Socket binding failed with error code: " << errno << "\n";
		}
		else {
			std::cout << "Successfully bound primary socket to: " << localIP << " - " << primPort << "\n";
		}
		sockaddr_in secSocketAddr = createSockAddr(localIP, secPort);
		resultCode = bind(secSocket, (const sockaddr*)&secSocketAddr, (socklen_t)sizeof(secSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "Secondary Socket binding failed with error code: " << errno << "\n";
		}
		else {
			std::cout << "Successfully bound secondary socket to: " << localIP << " - " << secPort << "\n";
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
		std::cout << "Connecting device (primary socket): " << SockAddrToStr(primConn) << "\n";
		connected.store(true);
		// NOTE: connect() unnecessary as transmit() uses sendto with primConn...
		int resultCode = connect(primSocket, (sockaddr*)&primConn, sizeof(primConn));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "Failed to connect primary socket with error: " << errno << "\n";
			connected.store(false);
		}
		else {
			std::cout << "Successfully connected. Primary socket connection: " << SockAddrToStr(primConn) 
				<< "Secondary socket connection: " << SockAddrToStr(secConn) << "\n";
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
			std::cout << "Data too large to send. Max packet size: " << MAX_PACKET_SIZE << ", length: " << length << "\n";
			return;
		}

		if (!connected.load()) {
			std::cout << "Tried to transmit but no connected device.\n";
			return;
		}

		lock.lock();

		int resultCode = sendto(primSocket, data, length, 0, (sockaddr*)&primConn, sizeof(primConn));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "sendto failed with error: " << errno << "\n";
		}

		lock.unlock();

		std::cout << "Successfully sent transmission to: " << SockAddrToStr(primConn) << "\n";
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
		std::cout << "Starting listening (blocking) on secondary socket on dedicated thread...\n";

		while (listening.load()) {
			// receive datagram
			int resultCode = recvfrom(secSocket, recvBuf, bufLen, 0, (sockaddr*)&sender, (socklen_t*)&senderSize);
			if (resultCode == SOCKET_ERROR) {
				std::cout << "Error receiving on secondary socket, code: " << errno << "\n";
			}
			else {
				// debug
				std::cout << "Received message: " << recvBuf << ", from: " <<
					SockAddrToStr(sender).c_str() << " - " << sender.sin_port << "\n";

				// check sender validity
				if (connected.load() && SockAddrToStr(sender) != SockAddrToStr(secConn)) {
					std::cout << "Received packet not originating from connected device. Connected to (secondary socket): "
						<< SockAddrToStr(secConn).c_str() <<
						", received from: " << SockAddrToStr(sender).c_str() << "\n";
				}
				else {
					handlePacket(recvBuf, sender);
				}
			}
		}

		std::cout << "Listening stopped...\n";
	}

	// handlePacket
	void handlePacket(char recvBuf[], sockaddr_in sender) {
		// check command
		int command;
		Serializer::deserializeInt(&command, (uint8_t*)recvBuf);

		// debug
		std::cout << "Recieved: " << recvBuf << "\n";

		if (command == CONNECT) {
			// parse request details
			int port; Serializer::deserializeInt(&port, (uint8_t*)recvBuf + 4);
			sockaddr_in newSecConn = createSockAddr(sender.sin_addr, sender.sin_port);
			sockaddr_in newPrimConn = createSockAddr(sender.sin_addr, port);
			std::cout << "Received connection request: " << SockAddrToStr(newPrimConn) << " / " 
				<< SockAddrToStr(newSecConn) << "\n";

			// connect to device
			connectDevice(newPrimConn, newSecConn);
		}
		else if (command == DISCONNECT) {
			disconnectDevice();
		}
		else {
			std::cout << "Received unrecognized command: " << command << "\n";
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
		sprintf(result, "%s-%d", str, sa.sin_port);
		return result;
	}
};

#endif // TRANSMITTER
