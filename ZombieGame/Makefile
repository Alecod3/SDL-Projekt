# Makefile for Windows

SRCDIR = ./source
CC = gcc
INCLUDE = C:\msys64\mingw64\include\SDL2

CFLAGS = -g -I$(INCLUDE) -c
LDFLAGS = -lmingw32 -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2 -mwindows -lm

OBJS = main.o powerups.o player.o mob.o sound.o

Game: $(OBJS)
	$(CC) $(OBJS) -o Game $(LDFLAGS)

main.o: $(SRCDIR)/main.c
	$(CC) $(CFLAGS) $(SRCDIR)/main.c -o main.o

powerups.o: $(SRCDIR)/powerups.c
	$(CC) $(CFLAGS) $(SRCDIR)/powerups.c -o powerups.o

player.o: $(SRCDIR)/player.c
	$(CC) $(CFLAGS) $(SRCDIR)/player.c -o player.o

mob.o: $(SRCDIR)/mob.c
	$(CC) $(CFLAGS) $(SRCDIR)/mob.c -o mob.o

sound.o: $(SRCDIR)/sound.c
	$(CC) $(CFLAGS) $(SRCDIR)/sound.c -o sound.o

clean:
	del Game.exe
	del *.o
