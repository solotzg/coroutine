main : main.cpp coroutine.hpp
	g++ -g -Wall -std=c++0x -c main.cpp -o main.o
	g++ -o main.out main.o
clean :
	rm main.o main.out
