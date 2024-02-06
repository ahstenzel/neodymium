CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

all: ./src/nd.c ./src/main.c
	$(CC) ./src/nd.c ./src/main.c -o ./bin/nd $(CFLAGS)