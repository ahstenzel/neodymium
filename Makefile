CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -std=gnu99
LFLAGS = -lc -lncurses

neodymium: ./src/neo.c ./src/main.c
	$(CC) ./src/neo.c ./src/main.c -o ./bin/neo $(CFLAGS) $(LFLAGS)

install: neodymium
	install -m 0755 ./bin/neo /usr/bin
