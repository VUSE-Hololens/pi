// SimSensor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<iostream>

#include "SimulatedSensor.h"

// pre-declarations
void testSendingFrame();
void testSendingBasic();
void testListening();
void testSimSensor();
void printFrame(Frame f);

int main()
{
    // test sending frame
	testSendingFrame();
	
	// test basic sending
	//testSendingBasic();
	
	// test listening
	//testListening();
	
	// testSimSensor
	//testSimSensor();
	
	std::cout << "\n\nAny key to exit.";
	std::cin.get();
	return 0;
}

// testSendingFrame
// tests ability to send dummy frame to all registered clients
void testSendingFrame() {
	char localIP[] = "10.66.247.250";
	int localInPort = 1000;
	int localOutPort = 1001;

	int h = 10; int w = 10; int b = 1;
	float xfov = 20; float yfov = 10;
	
	std::cout << "Testing Frame Transmitter\n\n";
	Transform t1;
	SimSensor ss(t1, h, w, b, xfov, yfov);
	Transmitter t(localIP, localInPort, localOutPort);

	std::cout << "Transmitter created: local IP: " << localIP << ", local port (in): " << localInPort << ", local port (out): " << localOutPort;
	std::cout << ". Connections: " << t.registers() << "\n\n";

	while (true) {
		Frame dummyFrame = ss.Data();
		std::cout << "Sending dummy frame. Pixels: " << h << "x" << w << "x" << b << ", FOV: " << xfov << "x" << yfov;
		std::cout << "Data: " << (unsigned)dummyFrame.pixels[0] << ", " << (unsigned)dummyFrame.pixels[1] << ", " << (unsigned)dummyFrame.pixels[2] << ", " << (unsigned)dummyFrame.pixels[3] << "...\n";
		t.transmit(dummyFrame.toBuffer(), dummyFrame.bufferSize());
	}
}

// testSendingBasic
// tests ability to send dummy messages to all registered clients
void testSendingBasic() {
	char localIP[] = "10.66.247.250";
	int localInPort = 1000;
	int localOutPort = 1001;
	uint8_t message[2];
	message[0] = 0; message[1] = 255;

	int connections = 0;

	std::cout << "Testing Basic Transmitter Sending\n";

	Transmitter t(localIP, localInPort, localOutPort);
	connections = t.registers();

	std::cout << "Transmitter created: local IP: " << localIP << ", local port (in): " << localInPort << ", local port (out): " << localOutPort;
	std::cout << ". Connections: " << connections << "\n";
	std::cout << "Continuously transmitting \"" << std::hex << (int)message[0] << (int)message[1] << "\" to all clients.\n\n";
	std::cout << std::dec;

	while (true) {
		// report new connections
		if (t.registers() != connections) {
			connections = t.registers();
			sockaddr_in newConnect = t.newestConnection();
			wchar_t ipStr[16];
			InetNtop(AF_INET, &newConnect.sin_addr, ipStr, 16);
			std::cout << "Main thread: Connection registered (IP: "; std::wcout << ipStr; std::cout << ", Port: " << ntohs(newConnect.sin_port) << ")";
			std::cout << ". Connections: " << connections << "\n";
		}

		// send dummy message
		//uint8_t *messageBytes = reinterpret_cast<uint8_t*>(message);
		t.transmit(message, 2);
	}
}

// testListening
// tests ability for devices to register as connections
void testListening() {
	char localIP[] = "10.66.247.250";
	int localInPort = 1000;
	int localOutPort = 1001;

	int connections = 0;

	std::cout << "Testing Transmitter Listening\n";
	
	Transmitter t(localIP, localInPort, localOutPort);
	connections = t.registers();

	std::cout << "Transmitter created: local IP: " << localIP << ", local port (in): " << localInPort << ", local port (out): " << localOutPort;
	std::cout << ". Connections: " << connections << "\n";

	while (true) {
		if (t.registers() != connections) {
			connections = t.registers();
			sockaddr_in newConnect = t.newestConnection();
			wchar_t ipStr[16];
			InetNtop(AF_INET, &newConnect.sin_addr, ipStr, 16);
			std::cout << "Main thread: Connection registered (IP: "; std::wcout << ipStr; std::cout << ", Port: " << ntohs(newConnect.sin_port) << ")";
			std::cout << ". Connections: " << connections << "\n";
		}
	}
}

// Tests SimSensor/Frames objects
	// T1: SimSensor::Data()
	// T2: Frame::copyCtor, Frame::assignmentOp
	// T3: Frame::Dtor
void testSimSensor() {
	// Test 1: SimSensor data
	std::cout << "Test 1: SimSensor data\n";
	int h = 5; int w = 10; int b = 3;
	float xfov = 20; float yfov = 10;
	Transform t1;
	SimSensor ss(t1, h, w, b, xfov, yfov);
	std::cout << "Frame 1:\n";
	printFrame(ss.Data());
	std::cout << "Frame 2:\n";
	printFrame(ss.Data());
	std::cout << "\n\n";

	// Test 2: Frame copy, assignment
	std::cout << "Test 2: Frame copy, assignment\n";
	Frame f2a = ss.Data();
	Frame f2b(f2a);
	std::cout << "Frame A:\n";
	printFrame(f2a);
	std::cout << "Frame B:\n";
	printFrame(f2b);
	std::cout << "\n\n";

	// Test 3: leaks
	std::cout << "Test 3: leaks\n";
	int iters = 100000;
	printf("Creating/deleting %d frames... ", iters);
	for (int i = 0; i < iters; i++) {
		Frame f3 = ss.Data();
	}
	std::cout << "done.\n";
	std::cout << "\n\n";
}

void printFrame(Frame f) {
	std::cout << "Frame: ";
	std::cout << "Size: " << f.height << "x" << f.width << "x" << f.bands << "  ";
	std::cout << "FOV: " << f.FOVx << "x" << f.FOVy << std::endl;
	for (int k = 0; k < f.bands; k++) {
		std::cout << "Band " << k + 1 << ":" << std::endl;
		for (int j = 0; j < f.height; j++) {
			for (int i = 0; i < f.width; i++) {
				printf("%5d", (int)f.get(i, j, k));
			}
			std::cout << std::endl;
		}
	}
}