// NetDev_Host_Linux.cpp : Defines the entry point for the console application.
// FOR pi

#define SOCKET_ERROR (-1)
#define MAX_PACKET_SIZE (1024)

#include<iostream>
#include <string>

// networking dependencies
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "Transmitter.h"

// test methods
void SendUDP(std::string localIP, int localPort, std::string remoteIP, int remotePort);
void ReceiveUDP(std::string localIP, int localPort);
void WaitForConnects(std::string localIP, int localPort);

// helpers
void Listen(int inSocket, bool echo);
std::string SockAddrToStr(sockaddr_in sa);
sockaddr_in createSockAddr(std::string ip, int port);

int main()
{
	// connection protocol test
	std::string localIP = "10.67.134.150";
	int localUDPPort = 8888; int localTCPPort = 8889;
	WaitForConnects(localIP, localUDPPort, localTCPPort);
	
	/*// simple UDP receive & echo
	std::string localIP = "10.67.134.150";
	int localPort = 8888;
	std::cout << "Testing UDP as listener...\n";
	ReceiveUDP(localIP, localPort);
	*/
	
	/*// simple UDP send
	std::string localIP = "10.67.134.150";
	int localPort = 8889;
	std::string remoteIP = "10.66.247.250";
	int remotePort = 8888;
	std::cout << "Testing UDP as sender...\n";
	SendUDP(localIP, localPort, remoteIP, remotePort);
	*/

	// hold window open
	std::cout << "Any key to exit.\n";
	std::cin.get();
	
	return 0;
}

// createSockAddr
sockaddr_in createSockAddr(std::string ip, int port) {
	sockaddr_in result;
	result.sin_family = AF_INET;
	result.sin_addr.s_addr = inet_addr(ip.c_str());
	result.sin_port = htons(port);
	return result;
}

void Listen(int socket, bool echo) {
	char recvBuf[MAX_PACKET_SIZE];
	int bufLen = MAX_PACKET_SIZE;
	sockaddr_in sender;
	int senderSize = sizeof(sender);

	std::cout << "Socket listening for incoming transmissions...\n";
	
	while (true) {
		// receive datagram
		int resultCode = recvfrom(socket, recvBuf, bufLen, 0, (sockaddr*)&sender, (socklen_t*)&senderSize);
		if (resultCode == SOCKET_ERROR) {
			std::cout << "Error in recvfrom, code: " << errno << "\n";
		}
		else {
			std::cout << "Received message: " << recvBuf << ", from: " << 
				SockAddrToStr(sender).c_str() << " - " << sender.sin_port << "\n";

			if (echo) {
				// echo
				int resultCode = sendto(socket, recvBuf, bufLen, 0, (sockaddr*)&sender, (socklen_t)senderSize);
				if (resultCode == SOCKET_ERROR) {
					std::cout << "error echoing message: " << errno << "\n";
				}
				else {
					std::cout << "Successfully echoed message back to sender\n";
				}
			}
		}
	}
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

// sets up basic socket, sends UDP packet and listens for response
void SendUDP(std::string localIP, int localPort, std::string remoteIP, int remotePort) {
	// create inSocket (UDP)
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == SOCKET_ERROR) {
		std::cout << "socket construction failed with error code: " << errno << "\n";
		return;
	}
	else {
		std::cout << "Successfully constructed UDP socket\n";
	}

	// bind inSocket to local port
	sockaddr_in inSocketAddr = createSockAddr(localIP, localPort);
	int resultCode = bind(sock, (const sockaddr*)&inSocketAddr, (socklen_t)sizeof(inSocketAddr));
	if (resultCode == SOCKET_ERROR) {
		std::cout << "Socket binding failed with error code: " << errno << "\n";
	}
	else {
		std::cout << "Successfully bound socket to: " << localIP << " - " << localPort << "\n";
	}

	// send UDP
	char message[] = "test";
	sockaddr_in dest = createSockAddr(remoteIP, remotePort);
	int destSize = sizeof(dest);
	for (int i = 0; i < 100; i++) {
		resultCode = sendto(sock, message, 5, 0, (sockaddr*)&dest, (socklen_t)destSize);
		if (resultCode == SOCKET_ERROR) {
			std::cout << "Sendto failed with error code: " << errno << "\n";
		}
	}
	std::cout << "Successfully sent message: " << message << ", to: " << remoteIP << " - " << remotePort << "\n";

	Listen(sock, false);
}

// basic socket setup and listen. Echos receives messages back to sender
void ReceiveUDP(std::string localIP, int localPort) {
	// create inSocket (UDP)
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == SOCKET_ERROR) {
		std::cout << "socket construction failed with error code: " << errno << "\n";
		return;
	}
	else {
		std::cout << "Successfully constructed UDP socket\n";
	}

	// bind inSocket to local port
	sockaddr_in inSocketAddr = createSockAddr(localIP, localPort);
	int resultCode = bind(sock, (const sockaddr*)&inSocketAddr, (socklen_t)sizeof(inSocketAddr));
	if (resultCode == SOCKET_ERROR) {
		std::cout << "Socket binding failed with error code: " << errno << "\n";
	}
	else {
		std::cout << "Successfully bound socket to: " << localIP << " - " << localPort << "\n";
	}

	Listen(sock, true);
}

// setup sockets, wait for connection request. Establish connection, send simple TCP message
void WaitForConnects(std::string localIP, int localUDPPort, int localTCPPort) {
	// create transmitter
	std::cout << "Tester: Created Transmitter (" << localIP << "-" << localUDPPort << "/" << localTCPPort <<
		"). Connection status: " << trans.connectionDetails() << "\n";

	// wait for async transmitter setup to finish
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// block until connection 
	std::cout << "Tester: Waiting for connection request...\n";
	while (!trans.hasConnection()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	// send dummy message to client
	char message[10] = { 1,2,3 };
	trans.transmit(message, 10);
}