CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99
LFLAGS = -lc -lncurses

all: ./src/neo.c ./src/main.c
	$(CC) ./src/neo.c ./src/main.c -o ./bin/neo $(CFLAGS) $(LFLAGS)