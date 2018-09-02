// Transmitter.h
// Class for two way transmission of serialized data with single remote client.
// Acts as host. Continuosly listens, transmits on command.
// Thread-safe: can safely transmit and manage connections on separate threads.
// configuration: WINDOWS

#pragma once
#ifndef TRANSMITTER
#define TRANSMITTER

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include <vector>
#include <algorithm>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>

#include "Serializer.h"

const int MAX_PACKET_SIZE = 1024;

// transmission command codes
const int CONNECT = 1;
const int DISCONNECT = 2;

class Transmitter {
public:
	// constructor
	Transmitter(std::string _localIP, int _inPort, int _outPort) 
		: localIP(_localIP), inPort(_inPort), outPort(_outPort)
	{
		// setup Winsock
		WSADATA wsaData;
		int resultCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (resultCode != NO_ERROR) {
			wprintf(L"WSAStartup failed with error: %d\n", resultCode);
		}

		// create inSocket & outSocket (TCP for outSocket, UDP for inSocket)
		inSocket = outSocket = INVALID_SOCKET;
		inSocket = socket(AF_INET, SOCK_DGRAM, 0);
		outSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (inSocket == INVALID_SOCKET) {
			wprintf(L"inSocket construction failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
		}
		if (outSocket == INVALID_SOCKET) {
			wprintf(L"outSocket construction failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
		}

		// bind inSocket & outSocket to ports
		sockaddr_in inSocketAddr = createSockAddr(localIP, inPort);
		resultCode = bind(inSocket, (sockaddr*)&inSocketAddr, sizeof(inSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			wprintf(L"inSocket bind failed with error %u\n", WSAGetLastError());
			closesocket(inSocket);
			WSACleanup();
		}
		sockaddr_in outSocketAddr = createSockAddr(localIP, outPort);
		resultCode = bind(outSocket, (sockaddr*)&outSocketAddr, sizeof(outSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			wprintf(L"outSocket bind failed with error %u\n", WSAGetLastError());
			closesocket(outSocket);
			WSACleanup();
		}

		// set inSocket as non-blocking (on dedicated thread)
		u_long blockMode = 1; // non-zero indicates non-blocking
		resultCode = ioctlsocket(inSocket, FIONBIO, &blockMode);
		if (resultCode == SOCKET_ERROR) {
			wprintf(L"Configure inSocket to non-blocking failed with error %u\n", WSAGetLastError());
			closesocket(inSocket);
			WSACleanup();
		}

		// start listening on inSocket
		connected = false;
		listening = true;
		listeningThread = std::thread(std::bind(&Transmitter::listen, this));
	}

	// destructor
	~Transmitter() {
		listening = false;
		listeningThread.join();

		closesocket(inSocket);
		closesocket(outSocket);
		WSACleanup();
	}

	// connect
	void connectDevice(sockaddr_in _inConn, sockaddr_in _outConn) {
		lock.lock();
		
		outConnection = _outConn;
		inConnection = _inConn;

		// connect local outSocket
		std::cout << "Connecting device: " << SockAddrToStr(outConnection) << "\n";
		connected.store(true);
		int resultCode = connect(outSocket, (sockaddr*)&outConnection, sizeof(outConnection));
		if (resultCode == SOCKET_ERROR) {
			wprintf(L"connect function (outConnection) failed with error: %ld\n", WSAGetLastError());
			closesocket(outSocket);
			WSACleanup();
			connected.store(false);
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
			return SockAddrToStr(outConnection);
		}
		else {
			return "Not connected.";
		}
	}

	// transmit
	void transmit(char* data, int length) {
		if (length > MAX_PACKET_SIZE) {
			std::cout << "Data too large to send. Max packet size: " << MAX_PACKET_SIZE << "\n";
			return;
		}

		if (!connected.load()) {
			std::cout << "Tried to transmit but no connected device.\n";
			return;
		}

		lock.lock();

		int resultCode = sendto(outSocket, data, length, 0, (sockaddr*)&outConnection, sizeof(outConnection));
		if (resultCode == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			closesocket(outSocket);
			WSACleanup();
		}

		lock.unlock();
	}

private:
	// data
	sockaddr_in outConnection, inConnection;
	SOCKET inSocket, outSocket;
	std::string localIP;
	int inPort, outPort;

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

		std::cout << "Listening begun...\n";

		while (listening.load()) {
			// receive datagram
			int resultCode = recvfrom(inSocket, recvBuf, bufLen, 0, (sockaddr*)&sender, &senderSize);
			if (resultCode == SOCKET_ERROR) {
				if (WSAGetLastError() != WSAEWOULDBLOCK) {
					wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
				}
			}
			else {
				// check sender validity
				if (connected.load() && SockAddrToStr(sender) != SockAddrToStr(inConnection)) {
					std::cout << "Received packet not originating from connected device. Connected to: "
						<< SockAddrToStr(inConnection).c_str() <<
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
	void handlePacket(char recvBuf[MAX_PACKET_SIZE], sockaddr_in sender) {
		// check command
		int command;
		Serializer::deserializeInt(&command, (uint8_t*)recvBuf);

		// debug
		std::cout << "Recieved: " << recvBuf << "\n";

		if (command == CONNECT) {
			// parse request details
			int port; Serializer::deserializeInt(&port, (uint8_t*)recvBuf + 4);
			sockaddr_in newInConn = createSockAddr(sender.sin_addr, sender.sin_port);
			sockaddr_in newOutConn = createSockAddr(sender.sin_addr, port);
			std::cout << "Received connection request: " << SockAddrToStr(newInConn) << " / " 
				<< SockAddrToStr(newOutConn) << "\n";

			// connect to device
			connectDevice(newInConn, newOutConn);
		}
		else if (command == DISCONNECT) {
			disconnectDevice();
		}
		else {
			std::cout << "Received unrecognized command: " << command << "\n";
		}
	}

	// createSockAddr
	sockaddr_in createSockAddr(std::string ip, int port) {
		sockaddr_in result;
		result.sin_family = AF_INET;

		//result.sin_addr.s_addr = inet_addr(ip.c_str());
		std::wstring ip_w(ip.length(), L' ');
		std::copy(ip.begin(), ip.end(), ip_w.begin());
		const wchar_t* _ip = ip_w.c_str();
		int resultCode = InetPton(AF_INET, _ip, &result.sin_addr);
		if (resultCode != 1) {
			std::cout << "IP text to binary conversion failed with error code " << WSAGetLastError() << "\n";
		}

		result.sin_port = port;
		return result;
	}
	sockaddr_in createSockAddr(in_addr ip, int port) {
		sockaddr_in result;
		result.sin_family = AF_INET;
		result.sin_addr = ip;
		result.sin_port = port;
		return result;
	}

	// SockAddrToStr
	std::string SockAddrToStr(sockaddr_in sa) {
		char result[50];
		// convert IP to string
		char str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);
		// format return
		sprintf_s(result, 50, "%s-%d", str, sa.sin_port);
		return result;
	}
};

#endif TRANSMITTER
