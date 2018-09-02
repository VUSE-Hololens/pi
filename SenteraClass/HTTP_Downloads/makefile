
main: clean.main
	g++ -o main HTTPDownloader.cpp -l curl -l turbojpeg

main.arm: clean.arm
	arm-none-linux-gnueabi-g++ -o main.arm HTTPDownloader.cpp

clean: clean.main clean.arm

clean.main:
	rm -f main
 
clean.arm:
	rm -f camtest.main

all: clean main main.arm
