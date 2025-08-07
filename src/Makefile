CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -std=gnu99 -I../vendor -DVERSION=\"$(file <VERSION)\"
LFLAGS = -lc -lncurses

neodymium: neo.c main.c
	$(CC) neo.c main.c -o ../bin/neo $(CFLAGS) $(LFLAGS)

install: neodymium
	install -m 0755 ../bin/neo /usr/bin
