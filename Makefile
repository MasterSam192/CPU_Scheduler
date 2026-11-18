CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
SRC = src/process.c src/engine.c src/gantt.c src/main.c
BIN = scheduler

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -f $(BIN)

.PHONY: all clean
