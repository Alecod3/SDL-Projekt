CC = gcc
CFLAGS = -g -IC:\msys64\mingw64\include\SDL2
LDFLAGS = -lmingw32 -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_net -lSDL2 -mwindows -lm

all: Game

Game: main.o powerups.o player.o mob.o sound.o
	$(CC) main.o powerups.o player.o mob.o sound.o -o Game $(LDFLAGS)

main.o: ./source/main.c
	$(CC) $(CFLAGS) -c ./source/main.c -o main.o

powerups.o: ./source/powerups.c
	$(CC) $(CFLAGS) -c ./source/powerups.c -o powerups.o

player.o: ./source/player.c
	$(CC) $(CFLAGS) -c ./source/player.c -o player.o

mob.o: ./source/mob.c
	$(CC) $(CFLAGS) -c ./source/mob.c -o mob.o

sound.o: ./source/sound.c
	$(CC) $(CFLAGS) -c ./source/sound.c -o sound.o

clean:
	del *.o Game.exe