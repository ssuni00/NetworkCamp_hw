CC = gcc
CFLAGS = -Wall -g
TARGET = hw2_server
OBJS = hw2_server.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

hw2_server.o: hw2_server.c protocol.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
