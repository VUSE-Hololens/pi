CFLAGS = #`pkg-config --cflags opencv`
LIBS = #`pkg-config --libs opencv`


main : clean.main
	g++ $(CFLAGS) $(LIBS) -o main NetDev_Host_Linux.cpp Transmitter.h Serializer.h

clean.main:
	rm -f main

clean: clean.main

all: clean main
