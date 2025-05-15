CC = g++
CFLAGS = -Wall -O2 -std=c++17

SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = yftp_client

LIBS = -lpthread -lboost_system -lboost_program_options -ljsoncpp

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean