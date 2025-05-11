# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Libraries
LIBS = -lpthread -ljsoncpp -lyaml-cpp -ltinyxml2 -lspdlog -lboost_system -lfmt

# Source files and output
SRC = main.cpp
TARGET = main

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Run tests
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all test clean