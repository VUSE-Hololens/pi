// NetDev_Host.cpp : Defines the entry point for the console application.
// Computer based testing

#include <string>
#include <iostream>
#include <fstream>
#include <istream>

// networking dependencies
#include "stdafx.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "Transmitter.h"
#include "Serializer.h"

// test methods
void ReceiveUDP(std::string localIP, int localPort);
void SendUDP(std::string localIP, int localPort, std::string remoteIP, int remotePort);
void testTransRemote();
void testTransLocal();
void testSerializer();

// helpers
void Listen(SOCKET inSocket, bool echo);
std::string SockAddrToStr(sockaddr_in sa);
sockaddr_in createSockAddr(std::string ip, int port);
void sendMessage(Transmitter trans, char data[], int length);

int main()
{
	/*// simple UDP receive & echo
	std::string localIP = "10.66.247.250";
	int localPort = 8888;
	std::cout << "Testing UDP as listener...\n";
	ReceiveUDP(localIP, localPort);
	*/
	

	// simple UDP send
	std::string localIP = "10.66.247.250";
	int localPort = 8889;
	std::string remoteIP = "10.67.134.150";
	int remotePort = 8888;
	std::cout << "Testing UDP as sender...\n";
	SendUDP(localIP, localPort, remoteIP, remotePort);

	// hold window open
	std::cout << "Any key to exit.\n";
	std::cin.get();
	
	return 0;
}

// test transmitter with remote client
void testTransRemote() {
	// create transmitter
	std::string localIP = "10.66.247.250";
	int localInPort = 9001;
	int localOutPort = 9000;
	Transmitter trans(localIP, localInPort, localOutPort);
	std::cout << "Tester: Created Transmitter (" << localIP << "-" << localInPort << "/" << localOutPort << 
		"). Connection status: " << trans.connectionDetails() << "\n";

	// wait for async transmitter setup to finish
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// block until connection 
	std::cout << "Waiting for connection request...\n";
	while (!trans.hasConnection()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	// send dummy message to client
	char message[10] = { 1,2,3 };
	trans.transmit(message, 10);
}

// Tests transmitter using local simulated client.
// Tests: creation, connection, disconnection
// NOTE: transmission test failing due to tester threading bug
void testTransLocal() {
	// create transmitter
	std::string localIP = "10.66.247.250";
	int localInPort = 8888;
	int localOutPort = 8889;
	Transmitter trans(localIP, localInPort, localOutPort);
	std::cout << "Tester: Created Transmitter. Connection status: " << trans.connectionDetails() << "\n";

	// create simulated client
	int clientInPort = 9000; int clientOutPort = 9001;
	// create sockets
	SOCKET clientInSocket = INVALID_SOCKET;
	clientInSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientInSocket == INVALID_SOCKET) {
		wprintf(L"tmpSocket construction failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
	}
	SOCKET clientOutSocket = INVALID_SOCKET;
	clientOutSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientOutSocket == INVALID_SOCKET) {
		wprintf(L"tmpSocket construction failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
	}
	// bind sockets
	sockaddr_in clientInAddr = createSockAddr(localIP, clientInPort);
	int resultCode = bind(clientInSocket, (sockaddr*)&clientInAddr, sizeof(clientInAddr));
	if (resultCode == SOCKET_ERROR) {
		wprintf(L"clientInSocket bind failed with error %u\n", WSAGetLastError());
		closesocket(clientInSocket);
		WSACleanup();
	}
	sockaddr_in clientOutAddr = createSockAddr(localIP, clientOutPort);
	resultCode = bind(clientOutSocket, (sockaddr*)&clientOutAddr, sizeof(clientOutAddr));
	if (resultCode == SOCKET_ERROR) {
		wprintf(L"clientOutSocket bind failed with error %u\n", WSAGetLastError());
		closesocket(clientOutSocket);
		WSACleanup();
	}
	// setup clientInSocket to listen & accept host's connection
	listen(clientInSocket, SOMAXCONN);

	// wait for async transmitter setup to finish
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// send transmitter connection request
	uint8_t message[8];
	Serializer::serializeInt(message, CONNECT);
	Serializer::serializeInt(message + 4, clientInPort);
	std::cout << "Tester: Requesting connection to port " << clientInPort << "\n";
	sockaddr_in hostInAddr = createSockAddr(localIP, localInPort);
	resultCode = sendto(clientOutSocket, (char*)message, sizeof(message), 0, (sockaddr*)&hostInAddr, sizeof(hostInAddr));
	if (resultCode == SOCKET_ERROR) {
		wprintf(L"sendto transmitter failed with error: %d\n", WSAGetLastError());
		closesocket(clientOutSocket);
		WSACleanup();
	}
	// accept host's connection request (blocking)
	SOCKET resultSocket = accept(clientInSocket, NULL, NULL);
	if (resultSocket == INVALID_SOCKET) {
		wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
		closesocket(resultSocket);
		WSACleanup();
	}
	
	// check request
	std::cout << "Tester: Connection status: " << trans.connectionDetails() << "\n";

	// test transmission
	/* Threading not working...
	char testData[] = "test test"; int testLength = 9;
	char recvBuf[9];
	std::thread sendThread = std::thread(sendMessage, trans, testData, testLength);
	resultCode = recv(clientInSocket, recvBuf, 9, 0);
	if (resultCode == SOCKET_ERROR) {
		printf("recv failed: %d\n", WSAGetLastError());
	}
	std::cout << "Tester: Client received transmission from host: " << recvBuf << "\n";
	*/

	// disconnect
	Serializer::serializeInt(message, DISCONNECT);
	std::cout << "Tester: Requesting disconnection...\n";
	resultCode = sendto(clientOutSocket, (char*)message, sizeof(message), 0, (sockaddr*)&hostInAddr, sizeof(hostInAddr));
	if (resultCode == SOCKET_ERROR) {
		wprintf(L"sendto transmitter failed with error: %d\n", WSAGetLastError());
		closesocket(clientOutSocket);
		WSACleanup();
	}

	// check request
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Tester: Connection status: " << trans.connectionDetails() << "\n";

	// cleanup
	//sendThread.join();
	closesocket(clientInSocket);
	closesocket(clientOutSocket);
}

void testSerializer() {
	// int
	int test = 6;
	int tmp = 2;
	int* dest = &tmp;
	uint8_t mid[4];
	Serializer::serializeInt(mid, test);
	Serializer::deserializeInt(dest, mid);
	std::cout << "After serialization/de-serialization, dest: " << *dest << "\n";

	// IP
	std::string testIP = "12.34.56.78";
	std::string destIP = "";
	uint8_t midIP[INET_ADDRSTRLEN];
	Serializer::serializeIP(midIP, testIP);
	Serializer::deserializeIP(&destIP, midIP);
	std::cout << "After serialization/de-serialization, destIP: "<< destIP << "\n";
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

// basic socket setup and listen. Echos receives messages back to sender
void ReceiveUDP(std::string localIP, int localPort) {
	// setup Winsock
	WSADATA wsaData;
	int resultCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (resultCode != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %d\n", resultCode);
	}
	else {
		std::cout << "Successfully started Winsock\n";
	}

	// create inSocket (UDP)
	SOCKET inSocket = INVALID_SOCKET;
	inSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (inSocket == INVALID_SOCKET) {
		wprintf(L"inSocket construction failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
	}
	else {
		std::cout << "Successfully constructed UDP socket\n";
	}

	// bind inSocket to local port
	sockaddr_in inSocketAddr = createSockAddr(localIP, localPort);
	resultCode = bind(inSocket, (sockaddr*)&inSocketAddr, sizeof(inSocketAddr));
	if (resultCode == SOCKET_ERROR) {
		wprintf(L"inSocket bind failed with error %u\n", WSAGetLastError());
		closesocket(inSocket);
		WSACleanup();
	}
	else {
		std::cout << "Successfully bound socket: " << localIP << " - " << localPort << "\n";
	}

	Listen(inSocket, true);
}

void Listen(SOCKET socket, bool echo) {
	char recvBuf[MAX_PACKET_SIZE];
	int bufLen = MAX_PACKET_SIZE;
	sockaddr_in sender;
	int senderSize = sizeof(sender);

	std::cout << "Socket listening for incoming transmissions...\n";
	
	while (true) {
		// receive datagram
		int resultCode = recvfrom(socket, recvBuf, bufLen, 0, (sockaddr*)&sender, &senderSize);
		if (resultCode == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			}
		}
		else {
			std::cout << "Received message: " << recvBuf << ", from: " << 
				SockAddrToStr(sender).c_str() << " - " << sender.sin_port << "\n";

			if (echo) {
				// echo
				int resultCode = sendto(socket, recvBuf, bufLen, 0, (sockaddr*)&sender, senderSize);
				for (int i = 0; i < 100; i++) {
					if (resultCode == SOCKET_ERROR) {
						wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
						closesocket(socket);
						WSACleanup();
					}
				}
				std::cout << "Successfully echoed message back to sender\n";
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
	sprintf_s(result, 50, "%s-%d", str, sa.sin_port);
	return result;
}

void sendMessage(Transmitter trans, char data[], int length) {
	// delay
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// send message
	trans.transmit(data, length);
}

// sets up basic socket, sends UDP packet and listens for response
void SendUDP(std::string localIP, int localPort, std::string remoteIP, int remotePort) {
	// setup Winsock
	WSADATA wsaData;
	int resultCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (resultCode != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %d\n", resultCode);
	}
	else {
		std::cout << "Successfully started Winsock\n";
	}

	// create inSocket (UDP)
	SOCKET sock = INVALID_SOCKET;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		wprintf(L"socket construction failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
	}
	else {
		std::cout << "Successfully constructed UDP socket\n";
	}

	// bind inSocket to local port
	sockaddr_in inSocketAddr = createSockAddr(localIP, localPort);
	resultCode = bind(sock, (sockaddr*)&inSocketAddr, sizeof(inSocketAddr));
	if (resultCode == SOCKET_ERROR) {
		wprintf(L"socket bind failed with error %u\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
	}
	else {
		std::cout << "Successfully bound socket: " << localIP << " - " << localPort << "\n";
	}

	// send UDP
	char message[] = "test";
	sockaddr_in dest = createSockAddr(remoteIP, remotePort);
	int destSize = sizeof(dest);
	for (int i = 0; i < 100; i++) {
		resultCode = sendto(sock, message, 5, 0, (sockaddr*)&dest, destSize);
		if (resultCode == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			closesocket(sock);
			WSACleanup();
		}
	}
	std::cout << "Successfully sent message: " << message << ", to: " << remoteIP << " - " << remotePort << "\n";

	Listen(sock, false);
}