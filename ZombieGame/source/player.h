#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include "powerups.h"

// Definiera konstanter om de inte redan finns globalt
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 30

// ADT för spelaren
typedef struct {
    SDL_Rect rect;
    int speed;
    int damage;
    int lives;
    int ammo;
} Player;

// Funktioner för att skapa, uppdatera och rita spelaren
Player create_player(int x, int y, int size, int speed, int damage, int lives, int ammo);
void update_player(Player *p, const Uint8 *state);
void draw_player(SDL_Renderer *renderer, const Player *p);
void draw_powerup_bars(SDL_Renderer *renderer, const Player *p, Powerup powerups[], int now);

extern SDL_Texture* tex_player;

#endif 