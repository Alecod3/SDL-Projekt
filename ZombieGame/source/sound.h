// sound.h
#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL_mixer.h>

// Opaque ADT for the sound system
typedef struct SoundSystem SoundSystem;

// Enumerates the different sound effects
typedef enum
{
    SOUND_SHOOT,
    SOUND_EXTRALIFE,
    SOUND_SPEED,
    SOUND_FREEZE,
    SOUND_DAMAGE,
    SOUND_COUNT
} SoundEffect;

// Create and initialize a new SoundSystem
// Returns NULL on failure.
SoundSystem *SoundSystem_create();

// Play looping background music from file
void SoundSystem_playMusic(SoundSystem *sys, const char *filename);

// Play a one‑shot sound effect
void SoundSystem_playEffect(SoundSystem *sys, SoundEffect effect);

// Destroy and free all resources
void SoundSystem_destroy(SoundSystem *sys);

#endif // SOUND_H
