#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL_mixer.h>

typedef enum {
    SOUND_SHOOT,
    SOUND_EXTRALIFE,
    SOUND_SPEED,
    SOUND_FREEZE,
    SOUND_DAMAGE,
    SOUND_COUNT 
} SoundEffect;

void init_sound();
void play_music(const char *filename);
void play_sound(SoundEffect effect);
void cleanup_sound();

#endif

