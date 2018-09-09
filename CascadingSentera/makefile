CFLAGS = #`pkg-config --cflags opencv`
LIBS = #`pkg-config --libs opencv`


main : clean.main
	g++ $(CFLAGS) $(LIBS) -o main main.cpp Transmitter.h Serializer.h  SenteraDouble4k.cpp compress.cpp DataPacketizer.cpp Bufferizer.cpp -l pthread -lturbojpeg -lcurl

clean.main:
	rm -f main

clean: clean.main

all: clean main

Bufferizer.o: Bufferizer.h Bufferizer.cpp
