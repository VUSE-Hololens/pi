
camtest: clean.camtest
	g++ -o camtest CameraTest.cpp crc.c Bufferizer.cpp Packetizer.cpp

camtest.arm: clean.arm
	arm-none-linux-gnueabi-g++ -o camtest.arm CameraTest.cpp crc.c Bufferizer.cpp Packetizer.cpp

camtest.android: clean.android
	export ARCH=arm
	arm-eabi-g++ -o cameratest.android CameraTest.cpp crc.c Bufferizer.cpp Packettizer.cpp

clean: clean.camtest clean.arm clean.android

clean.camtest:
	rm -f camtest
 
clean.arm:
	rm -f camtest.arm

clean.android:
	rm -f camtest.android

all: clean camtest camtest.arm

Bufferizer.o: Bufferizer.cpp Bufferizer.h crc.h suas_types.h 