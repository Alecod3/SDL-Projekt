#ifndef SOUND_H
#define SOUND_H

#include <SDL_mixer.h>

void init_sound();
void play_music(const char *filename);
void cleanup_sound();

#endif
