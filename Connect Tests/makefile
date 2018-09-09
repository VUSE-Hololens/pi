CFLAGS = #`pkg-config --cflags opencv`
LIBS = #`pkg-config --libs opencv`


main : clean.main
	g++ $(CFLAGS) $(LIBS) -pthread -o main NetDev_Tester.cpp Transmitter.h Serializer.h

clean.main:
	rm -f main

clean: clean.main

all: clean main
