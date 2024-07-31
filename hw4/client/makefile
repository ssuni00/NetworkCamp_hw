CC = gcc
CFLAGS = -I/home/s22200809/local/include -I/home/s22200809/local/include/ncurses
LDFLAGS = -L/home/s22200809/local/lib -lncurses

TARGET = client

SRC = client.c
HEADERS = client.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -f *.o
