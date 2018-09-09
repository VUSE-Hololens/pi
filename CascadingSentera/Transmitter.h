// Transmitter.h
// Class for two way transmission of serialized data with single remote client.
// Acts as host. Continuosly listens, transmits on command.
// Thread-safe: can safely transmit and manage connections on separate threads.
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

const int MAX_PACKET_SIZE = 35000;
const int SOCKET_ERROR = -1;
const int WOULD_BLOCK = 11;
const int HEADER_SIZE = 12;// bytes, 3 int32's (message length, width, height)

// transmission command codes
const int CONNECT = 1;
const int DISCONNECT = 2;

class Transmitter {
public:
	// constructor
	Transmitter(std::string _localIP, int _UDPPort, int _TCPPort) 
		: localIP(_localIP), UDPPort(_UDPPort), TCPPort(_TCPPort)
	{
		// create sockets
		UDPSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
		if (UDPSocket == SOCKET_ERROR) {
			std::cout << "UDP socket construction failed with error code: " << errno << "\n";
			return;
		}
		else {
			std::cout << "Successfully constructed UDP socket (non-blocking)\n";
		}
		TCPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (TCPSocket == SOCKET_ERROR) {
			std::cout << "TCP socket construction failed with error code: " << errno << "\n";
			return;
		}
		else {
			std::cout << "Successfully constructed TCP socket\n";
		}

		// bind  UDP Socket to local port
		sockaddr_in UDPSocketAddr = createSockAddr(localIP, UDPPort);
		int resultCode = bind(UDPSocket, (const sockaddr*)&UDPSocketAddr, (socklen_t)sizeof(UDPSocketAddr));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "UDP Socket binding failed with error code: " << errno << "\n";
		}
		else {
			std::cout << "Successfully bound UDP socket to: " << localIP << " - " << UDPPort << "\n";
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

		close(UDPSocket);
		close(TCPSocket);
	}

	// connect
	void connectDevice(sockaddr_in _inConn, sockaddr_in _outConn) {
		lock.lock();
		
		outConnection = _outConn;
		inConnection = _inConn;

		// connect local outSocket
		std::cout << "Connecting device: " << SockAddrToStr(outConnection) << "\n";
		connected.store(true);
		int resultCode = connect(TCPSocket, (sockaddr*)&outConnection, sizeof(outConnection));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "Failed to connect with error: " << errno << "\n";
			close(TCPSocket);
			connected.store(false);
		}
		else {
			std::cout << "Successfully connected to: " << SockAddrToStr(outConnection) << "\n";
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

		int resultCode = sendto(TCPSocket, data, length, 0, (sockaddr*)&outConnection, sizeof(outConnection));
		if (resultCode == SOCKET_ERROR) {
			std::cout << "sendto failed with error: " << errno << "\n";
			close(TCPSocket);
		}

		lock.unlock();

		std::cout << "Successfully sent transmission to: " << SockAddrToStr(outConnection) << "\n";
	}

private:
	// data
	sockaddr_in outConnection, inConnection;
	int UDPSocket, TCPSocket;
	std::string localIP;
	int UDPPort, TCPPort;

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

		std::cout << "Listening started...\n";

		while (listening.load()) {
			// receive datagram
			int resultCode = recvfrom(UDPSocket, recvBuf, bufLen, 0, (sockaddr*)&sender, (socklen_t*)&senderSize);
			if (resultCode == SOCKET_ERROR && errno != WOULD_BLOCK) {
				std::cout << "Error in recvfrom, code: " << errno << "\n";
			}
			else if (resultCode != SOCKET_ERROR) {
				std::cout << "Received message: " << recvBuf << ", from: " <<
					SockAddrToStr(sender).c_str() << " - " << sender.sin_port << "\n";

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