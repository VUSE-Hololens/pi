
camtest: clean.camtest
	g++ -o camtest CameraTest.cpp Bufferizer.cpp DataPacketizer.cpp

camtest.arm: clean.arm
	arm-none-linux-gnueabi-g++ -o camtest.arm CameraTest.cpp Bufferizer.cpp DataPacketizer.cpp

camtest.android: clean.android
	export ARCH=arm
	arm-eabi-g++ -o cameratest.android CameraTest.cpp Bufferizer.cpp DataPacketizer.cpp

clean: clean.camtest clean.arm clean.android

clean.camtest:
	rm -f camtest
 
clean.arm:
	rm -f camtest.arm

clean.android:
	rm -f camtest.android

all: clean camtest camtest.arm

Bufferizer.o: Bufferizer.cpp Bufferizer.h 
