prefix = /usr
CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -std=gnu99 -I../vendor -I../vendor/vex/include -DVERSION=\"$(file <VERSION)\"
LFLAGS = -lc -lncurses

all: neodymium

debug: CFLAGS += -DDEBUG -g
debug: all

neodymium: neo.c main.c
	$(CC) neo.c main.c -o neo $(CFLAGS) $(LFLAGS)

clean:
	-rm -f neo

distclean: clean

install: neodymium
	install -m 0755 -D neo $(DESTDIR)$(prefix)/bin/neo

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/neo

.PHONY: all debug install clean distclean uninstall