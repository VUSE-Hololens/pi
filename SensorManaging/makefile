
camtest: clean.camtest
	g++ -o camtest main.cpp SenteraDouble4k.cpp Bufferizer.cpp DataPacketizer.cpp -l curl -l turbojpeg

clean: clean.camtest

clean.camtest:
	rm -f camtest

all: clean camtest 

Bufferizer.o: Bufferizer.cpp Bufferizer.h 
