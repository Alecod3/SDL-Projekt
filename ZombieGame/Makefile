# Makefile for Windows
SRCDIR=.\source
CC=gcc
INCLUDE = C:\msys64\mingw64\include\SDL2

CFLAGS = -g -I$(INCLUDE) -c 
LDFLAGS = -lmingw32 -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2 -mwindows -lm

Game: main.o
	$(CC) main.o -o Game $(LDFLAGS)

main.o: $(SRCDIR)\main.c
	$(CC) $(CFLAGS) $(SRCDIR)\main.c

clean:
	rm *.exe
	rm *.o
