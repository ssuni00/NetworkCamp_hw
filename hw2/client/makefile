CC = gcc
CFLAGS = -Wall -g
TARGET = hw2_client
OBJS = hw2_client.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

hw2_client.o: hw2_client.c protocol.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
