#include "sound.h"
#include <stdio.h>

Mix_Music *bgMusic = NULL;
Mix_Chunk *soundEffects[SOUND_COUNT] = {NULL};

void init_sound(){
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer init error: %s\n", Mix_GetError());
    }
    soundEffects[SOUND_SHOOT]     = Mix_LoadWAV("source/shoot.wav");
    soundEffects[SOUND_EXTRALIFE] = Mix_LoadWAV("source/life1.wav");
    soundEffects[SOUND_SPEED]     = Mix_LoadWAV("source/speed.wav");
    soundEffects[SOUND_FREEZE]    = Mix_LoadWAV("source/freeze.wav");
    soundEffects[SOUND_DAMAGE]    = Mix_LoadWAV("source/ddamage.wav");
}

void play_music(const char *filename){
    bgMusic = Mix_LoadMUS(filename);
    if(!bgMusic){
        printf("Misslyckades med att ladda musik: %s\n", Mix_GetError());
    }else{
        Mix_PlayMusic(bgMusic, -1);
    }
}

void play_sound(SoundEffect effect){
    if(effect >= 0 && effect < SOUND_COUNT && soundEffects[effect]){
        Mix_PlayChannel(-1, soundEffects[effect], 0);
    }
}

void cleanup_sound(){
    for(int i = 0; i < SOUND_COUNT; i++){
        if(soundEffects[i]){
            Mix_FreeChunk(soundEffects[i]);
            soundEffects[i] = NULL;
        }
    }
    if(bgMusic){
        Mix_FreeMusic(bgMusic);
        bgMusic = NULL;
    }
    Mix_CloseAudio();
}
