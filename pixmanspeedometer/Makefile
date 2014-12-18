CC = gcc -Wall -pedantic -ansi -g

CFLAGS=$(shell pkg-config --cflags pixman-1)
LIBS=$(shell pkg-config --libs pixman-1)

all:
	$(CC) main.c -o pixmantest $(CFLAGS) $(LIBS)

clean:
	@echo Cleaning up...
	@rm pixmantest
	@echo Done.
