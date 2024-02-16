CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LFLAGS = -lc -lncurses

all: ./src/nd.c ./src/main.c
	$(CC) ./src/nd.c ./src/main.c -o ./bin/nd $(CFLAGS) $(LFLAGS)