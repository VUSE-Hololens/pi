
camtest: clean.camtest
	g++ -o camtest main.cpp SenteraDouble4k.cpp Bufferizer.cpp DataPacketizer.cpp

camtest.arm: clean.arm
	arm-none-linux-gnueabi-g++ -o camtest.arm main.cpp SenteraDouble4k.cpp Bufferizer.cpp DataPacketizer.cpp

camtest.android: clean.android
	export ARCH=arm
	arm-eabi-g++ -o cameratest.android main.cpp SenteraDouble4k.cpp Bufferizer.cpp DataPacketizer.cpp

clean: clean.camtest clean.arm clean.android

clean.camtest:
	rm -f camtest
 
clean.arm:
	rm -f camtest.arm

clean.android:
	rm -f camtest.android

all: clean camtest camtest.arm

Bufferizer.o: Bufferizer.cpp Bufferizer.h 
