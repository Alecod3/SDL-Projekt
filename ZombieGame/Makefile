CC = gcc
SRCDIR = ./source
INCDIR = ./source

INCLUDE = /opt/homebrew/include/SDL2
LIBS = /opt/homebrew/lib

CFLAGS = -g -I$(INCLUDE) -c
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm

OBJS = main.o powerups.o player.o mob.o sound.o

Game: $(OBJS)
	$(CC) -o Game $(OBJS) $(LDFLAGS) -L $(LIBS)

main.o: $(SRCDIR)/main.c $(INCDIR)/player.h $(INCDIR)/mob.h $(INCDIR)/powerups.h
	$(CC) $(CFLAGS) $(SRCDIR)/main.c -o main.o

powerups.o: $(SRCDIR)/powerups.c $(INCDIR)/powerups.h
	$(CC) $(CFLAGS) $(SRCDIR)/powerups.c -o powerups.o

player.o: $(SRCDIR)/player.c $(INCDIR)/player.h
	$(CC) $(CFLAGS) $(SRCDIR)/player.c -o player.o

mob.o: $(SRCDIR)/mob.c $(INCDIR)/mob.h
	$(CC) $(CFLAGS) $(SRCDIR)/mob.c -o mob.o

sound.o: $(SRCDIR)/sound.c $(INCDIR)/sound.h
	$(CC) $(CFLAGS) $(SRCDIR)/sound.c -o sound.o

clean:
	rm -f Game
	rm -f *.o

