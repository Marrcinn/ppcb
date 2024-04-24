CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -O3

ppcbc: ppcbc.o common.o
	$(CXX) $(CXXFLAGS) -o ppcbc ppcbc.o common.o

ppcbs: ppcbs.o common.o
	$(CXX) $(CXXFLAGS) -o ppcbs ppcbs.o common.o

ppcbc.o: ppcbc.cpp common.h
	$(CXX) $(CXXFLAGS) -c ppcbc.cpp

ppcbs.o: ppcbs.cpp common.h
	$(CXX) $(CXXFLAGS) -c ppcbs.cpp

common.o: common.cpp common.h
	$(CXX) $(CXXFLAGS) -c common.cpp

all: ppcbc ppcbs


clean:
	rm -f ppcbc ppcbs *.o

