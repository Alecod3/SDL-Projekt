#ifndef SOUND_H
#define SOUND_H

#include <SDL_mixer.h>

void init_sound();
void play_music(const char *filename);
void cleanup_sound();
void play_shoot_sound();
void play_extralife_sound();
void play_speed_sound();
void play_freeze_sound();
void play_damage_sound();

#endif
