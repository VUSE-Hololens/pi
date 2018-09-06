// NetDev_Tester.cpp : Testing of NetDev host code
// FOR pi

#include <iostream>
#include <string>
#include <exception>

// networking dependencies
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Transmitter.h"
#include "Serializer.h"

// test methods
void testTrans();

// helpers
std::string SockAddrToStr(sockaddr_in sa);
sockaddr_in createSockAddr(std::string ip, int port);

int main()
{
	// testTrans
	testTrans();

	// hold window open
	std::cout << "Any key to exit.\n";
	std::cin.get();

	return 0;
}

// testTrans
// tests Transmitter class
void testTrans() {
	// create transmitter: creates, binds sockets, starts listening
	std::string localIP = "10.67.134.150"; // not verified
	int localPrimPort = 8888;
	int localSecPort = 8889;
	Transmitter trans(localIP, localPrimPort, localSecPort);

	// if connected, try transmission
	while (true) {
		if (trans.hasConnection()) {
			// add message length
			uint8_t message[100];
			int messageLength = 100;
			Serializer::serializeInt(message, messageLength);

			// add header
			int headerVal1 = 50;
			int headerVal2 = 60;
			Serializer::serializeInt(message + 4, headerVal1);
			Serializer::serializeInt(message + 8, headerVal2);

			// add body
			for (int i = 9; i < messageLength; i++) {
				message[i] = i;
			}

			// send message
			trans.transmit((char*)message, messageLength);
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
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