CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra

TARGET = indexer_daemon
SRC = indexer_daemon.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
