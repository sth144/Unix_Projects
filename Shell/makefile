CC=gcc
CFLAGS=-std=c99

all: engine.c commandLoop.h commandLoop.c stack.h stack.c
	$(CC) engine.c commandLoop.c stack.c -o smallsh $(CFLAGS)

test:
	./p3testscript 2>&1

backup:
	cp * ../backups/p3backup

clean:
	rm smallsh
	rm junk*
	rm badfile
	rm ../../testdir*
	echo "" > newOut.txt
