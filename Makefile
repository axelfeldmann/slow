# Name of the output executable
OUTPUT = slow

# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -fpermissive -g

# Source files
SOURCES = slow.cpp primes.cpp

# Make rules

# Default target
all: $(OUTPUT)

$(OUTPUT): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(OUTPUT)

# Clean rule to remove compiled files
clean:
	rm -f $(OUTPUT)

