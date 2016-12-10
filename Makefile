CXX = g++
CXXFLAGS = -g -std=c++11
THREAD = -pthread

all:  test1 test2 test3

test1: hashmap.h
	$(CXX) $(CXXFLAGS) $(THREAD) test1.cpp -o test1

test2: hashmap.h
	$(CXX) $(CXXFLAGS) $(THREAD) test2.cpp -o test2

test3: hashmap.h
	$(CXX) $(CXXFLAGS) $(THREAD) test3.cpp -o test3

clean:
	-rm test1 test2 test3
