 
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lmosquitto -ljansson

SRC = lantern.c
TARGET = lantern

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
