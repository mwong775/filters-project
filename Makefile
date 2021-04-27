CC = g++

CFLAGS = -pg -g -Wall -std=c++14 -mpopcnt -march=native

LDFLAGS+= -Wall -lpthread -lssl -lcrypto

all: bfc cp vp

bfc : bfc.cpp bf_cascade/bf_cascade.h
	g++ $(CFLAGS) -Ofast -o bfc bfc.cpp

cp : cp.cc cuckoopair.hh
	g++ $(CFLAGS) -Ofast -o cp cp.cc  

vp : vp.cc vacuumpair.hh
	g++ $(CFLAGS) -Ofast -o vp vp.cc  

clean:
	rm -f bfc
	rm -f cp
	rm -f vp

# not used yet - still need final testing files