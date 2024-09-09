CXX = g++
CXXFLAGS = -Wall -Werror -std=c++17

SOURCES = $(wildcard *.cpp)
OBJECTS = $(filter-out slow.o slower.o, $(SOURCES:.cpp=.o))

all: slow slower

slow: slow.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

slower: slower.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f slow slower $(OBJECTS) slow.o slower.o

.PHONY: all clean