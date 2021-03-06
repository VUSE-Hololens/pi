// NDVICalcTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DataProcessor.h"

#include <cstdint>
#include <Random>
#include <iostream>

// pre-declaration
void printFrameData(Frame* f);

int main()
{
    // controls
	const int height = 3;
	const int width = 3;
	const int bands = 3;

	// setup random number gen
	/*
	std::random_device rd;     // only used once to initialise (seed) engine
	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, 255); // guaranteed unbiased
	*/
	
	// create dummy pixels
	uint8_t* rgb_pixels = new uint8_t[height*width*bands];
	uint8_t* nir_pixels = new uint8_t[height*width*bands];
	for (int i = 0; i < height*width*bands; i++) {
		//rgb_pixels[i] = (uint8_t)uni(rng);
		//nir_pixels[i] = (uint8_t)uni(rng);
		rgb_pixels[i] = i;
		nir_pixels[i] = i;
	}

	// create dummy Frames
	Frame rgb(height, width, bands, 0, 0); Frame nir(height, width, bands, 0, 0);
	rgb.pixels = rgb_pixels; nir.pixels = nir.pixels;

	
	// show results
	std::cout << "RBG: "; printFrameData(&rgb);
	std::cout << "NIR: "; printFrameData(&nir);


	std::cout << "Any key to exit:\n";
	std::cin.get();
	return 0;
}

// printFrameData
void printFrameData(Frame* f) {
	int h = f->height; int w = f->width; int b = f->bands;
	std::cout << "height: " << h << ", width: " << w << ", bands: " << b << "\n";
	for (int k = 0; k < b; k++) {
		std::cout << "Band " << k << ":\n";
		for (int j = 0; j < h; j++) {
			for (int i = 0; i < w; i++) {
				std::cout << (int)f->get(i, j, k) << " ";
			}
			std::cout << "\n";
		}
	}
	std::cout << "\n\n";
}