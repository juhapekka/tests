CC = gcc -Wall -pedantic -ansi -g

all:
	$(CC) main.c -o textureuploadtest $(CFLAGS) $(LIBS) -I/opt/include -I/usr/include/pixman-1 -L/opt/lib -L/usr/lib/x86_64-linux-gnu -lX11 -lGLU -lGL -lpixman-1

clean:
	@echo Cleaning up...
	@rm textureuploadtest
	@echo Done.
