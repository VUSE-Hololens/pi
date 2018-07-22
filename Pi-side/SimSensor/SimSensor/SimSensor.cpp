// SimSensor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<iostream>

#include "SimulatedSensor.h"

// pre-declarations
void testSimSensor();
void printFrame(Frame f);

int main()
{
    // testSimSensor
	testSimSensor();
	
	std::cout << "\n\nAny key to exit.";
	std::cin.get();
	return 0;
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