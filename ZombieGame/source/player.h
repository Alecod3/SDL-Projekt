#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <stdbool.h>

// Definiera konstanter om de inte redan finns globalt
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 30
#define DEFAULT_PLAYER_SPEED 5.0f
#define DEFAULT_PLAYER_DAMAGE 1
#define MAX_HEALTH 3

// ADT för spelaren
typedef struct
{
    SDL_Rect rect;
    float speed;
    int damage;
    int lives;
    float angle;
} Player;

// Funktioner för att skapa, uppdatera och rita spelaren
Player create_player(int x, int y, int size, float speed, int damage, int lives);
void update_player(Player *player, const Uint8 *state);
void draw_player(SDL_Renderer *renderer, Player *player);

extern SDL_Texture *tex_player;

#endif