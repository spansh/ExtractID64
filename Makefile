.SUFFIXES:
MAKEFLAGS += -r
SHELL := /bin/bash
.DELETE_ON_ERROR:
.PHONY: all test clean


all: ExtractID64

%: %.cpp
	g++ -std=c++17 -O0 -g3 -ggdb -fno-eliminate-unused-debug-types -Wall -Wextra -pedantic -I../rapidjson/include/ -I../zstr/src -o $@ $^ -lz -lstdc++fs -static -L/usr/local/lib

clean: ExtractID64
	rm -rf $^

production:
	g++ -std=c++17 -O3 -msseregparm -flto -msse -msse2 -msse3 -msse4.2 -minline-all-stringops -mfpmath=sse -march=native -Wall -Wextra -pedantic -I../rapidjson/include/ -I../zstr/src -o ExtractID64  ExtractID64.cpp -lz -lstdc++fs -static -L/usr/local/lib
