#ifndef PLAYER_H
#define PLAYER_H
#include "powerups.h"

#include <SDL2/SDL.h>

// Definiera konstanter om de inte redan finns globalt
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 30

// ADT för spelaren
typedef struct Player Player;
typedef struct Powerup Powerup;

// Funktioner för att skapa, uppdatera och rita spelaren
Player *create_player(int x, int y, int size, int speed, int damage, int lives);
void update_player(Player *p, const Uint8 *state);
void draw_player(SDL_Renderer *renderer, const Player *p);
void draw_powerup_bars(SDL_Renderer *renderer, const Player *p, Powerup powerups[], int now);

// Getter/setter
SDL_Rect player_get_rect(const Player *p);
void player_set_position(Player *p, int x, int y);
int player_get_lives(const Player *p);
void player_set_lives(Player *p, int lives);
void player_set_tint(Player *p, SDL_Color tint);
int player_get_x(const Player *p);
int player_get_y(const Player *p);
int player_get_center_x(const Player *p);
int player_get_center_y(const Player *p);
float player_get_aim_angle(const Player *p);
int player_get_speed(const Player *p);
void player_set_speed(Player *p, int speed);
int player_get_damage(const Player *p);
void player_set_damage(Player *p, int damage);
void player_set_aim_angle(Player *p, float angle);

extern SDL_Texture *tex_player;

#endif