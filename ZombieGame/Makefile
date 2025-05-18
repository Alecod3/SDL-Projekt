SRCDIR = ./source
CC = gcc
INCLUDE = C:\msys64\mingw64\include\SDL2

CFLAGS = -g -I$(INCLUDE) -c
LDFLAGS = -lmingw32 -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2 -lSDL2_net -mwindows -lm

OBJS = main.o game.o menu.o powerups.o player.o mob.o sound.o network.o

Game: $(OBJS)
	$(CC) $(OBJS) -o Game $(LDFLAGS)

main.o: $(SRCDIR)/main.c
	$(CC) $(CFLAGS) $(SRCDIR)/main.c -o main.o

game.o: $(SRCDIR)/game.c
	$(CC) $(CFLAGS) $(SRCDIR)/game.c -o game.o

menu.o: $(SRCDIR)/menu.c
	$(CC) $(CFLAGS) $(SRCDIR)/menu.c -o menu.o

powerups.o: $(SRCDIR)/powerups.c
	$(CC) $(CFLAGS) $(SRCDIR)/powerups.c -o powerups.o

player.o: $(SRCDIR)/player.c
	$(CC) $(CFLAGS) $(SRCDIR)/player.c -o player.o

mob.o: $(SRCDIR)/mob.c
	$(CC) $(CFLAGS) $(SRCDIR)/mob.c -o mob.o

sound.o: $(SRCDIR)/sound.c
	$(CC) $(CFLAGS) $(SRCDIR)/sound.c -o sound.o

network.o: $(SRCDIR)/network.c
	$(CC) $(CFLAGS) $(SRCDIR)/network.c -o network.o

clean:
	del Game.exe
	del *.o