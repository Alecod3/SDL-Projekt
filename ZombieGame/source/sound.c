// sound.c
#include "sound.h"
#include <stdlib.h>
#include <stdio.h>

struct SoundSystem
{
    Mix_Music *bgMusic;
    Mix_Chunk *effects[SOUND_COUNT];
};

SoundSystem *SoundSystem_create()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        fprintf(stderr, "SDL_mixer init error: %s\n", Mix_GetError());
        return NULL;
    }
    SoundSystem *sys = malloc(sizeof(*sys));
    if (!sys)
        return NULL;

    sys->bgMusic = NULL;

    // Load effects
    sys->effects[SOUND_SHOOT] = Mix_LoadWAV("source/shoot.wav");
    sys->effects[SOUND_EXTRALIFE] = Mix_LoadWAV("source/life2.wav");
    sys->effects[SOUND_SPEED] = Mix_LoadWAV("source/speed.wav");
    if (sys->effects[SOUND_SPEED])
        Mix_VolumeChunk(sys->effects[SOUND_SPEED], 128);
    sys->effects[SOUND_FREEZE] = Mix_LoadWAV("source/freeze.wav");
    if (sys->effects[SOUND_FREEZE])
        Mix_VolumeChunk(sys->effects[SOUND_FREEZE], 128);
    sys->effects[SOUND_DAMAGE] = Mix_LoadWAV("source/ddamage.wav");
    if (sys->effects[SOUND_DAMAGE])
        Mix_VolumeChunk(sys->effects[SOUND_DAMAGE], 128);

    // Check for load errors
    for (int i = 0; i < SOUND_COUNT; i++)
    {
        if (!sys->effects[i])
        {
            fprintf(stderr, "Failed loading effect %d: %s\n", i, Mix_GetError());
        }
    }

    return sys;
}

void SoundSystem_playMusic(SoundSystem *sys, const char *filename)
{
    if (!sys)
        return;

    // Free previous music
    if (sys->bgMusic)
    {
        Mix_FreeMusic(sys->bgMusic);
        sys->bgMusic = NULL;
    }

    sys->bgMusic = Mix_LoadMUS(filename);
    if (!sys->bgMusic)
    {
        fprintf(stderr, "Failed to load music '%s': %s\n", filename, Mix_GetError());
        return;
    }

    Mix_VolumeMusic(18);
    if (Mix_PlayMusic(sys->bgMusic, -1) < 0)
    {
        fprintf(stderr, "Failed to play music: %s\n", Mix_GetError());
    }
}

void SoundSystem_playEffect(SoundSystem *sys, SoundEffect effect)
{
    if (!sys)
        return;
    if (effect < 0 || effect >= SOUND_COUNT)
        return;
    Mix_Chunk *chunk = sys->effects[effect];
    if (chunk)
    {
        if (Mix_PlayChannel(-1, chunk, 0) < 0)
        {
            fprintf(stderr, "Failed to play effect %d: %s\n", effect, Mix_GetError());
        }
    }
}

void SoundSystem_destroy(SoundSystem *sys)
{
    if (!sys)
        return;
    for (int i = 0; i < SOUND_COUNT; i++)
    {
        if (sys->effects[i])
        {
            Mix_FreeChunk(sys->effects[i]);
        }
    }
    if (sys->bgMusic)
    {
        Mix_FreeMusic(sys->bgMusic);
    }
    Mix_CloseAudio();
    free(sys);
}
