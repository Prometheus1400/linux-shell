# makefile

run: start
	./start

all: start

start: start.cpp
	g++ -g -w -std=c++11 -o start start.cpp shell_functions.cpp

clean:
	rm start
