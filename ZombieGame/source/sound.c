#include "sound.h"
#include <stdio.h>

Mix_Music *bgMusic = NULL;

void init_sound() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer init error: %s\n", Mix_GetError());
    }
}

void play_music(const char *filename) {
    bgMusic = Mix_LoadMUS(filename);
    if (!bgMusic) {
        printf("Misslyckades med att ladda musik: %s\n", Mix_GetError());
    } else {
        Mix_PlayMusic(bgMusic, -1);
    }
}

void cleanup_sound() {
    if (bgMusic != NULL) {
        Mix_FreeMusic(bgMusic);
    }
    Mix_CloseAudio();
}
