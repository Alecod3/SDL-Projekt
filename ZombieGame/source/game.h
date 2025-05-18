#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "player.h"
#include "mob.h"
#include "powerups.h"
#include "sound.h"
#include "network.h"

#define BULLET_SIZE 7
#define BULLET_SPEED 7
#define MAX_BULLETS 10
#define MOB_SPAWN_INTERVAL_MS 1500
#define MOBS_SPAWN_PER_WAVE 3
#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20

typedef struct
{
    SDL_Rect rect;
    bool active;
    float dx, dy;
} Bullet;

int run_game(SDL_Renderer *renderer, SDL_Window *window, TTF_Font *uiFont, SoundSystem *audio);

#endif