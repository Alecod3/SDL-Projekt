#ifndef POWERUPS_H
#define POWERUPS_H
#define PLAYER_SPEED 5
#define DEFAULT_PLAYER_SPEED 5

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum {
    POWERUP_EXTRA_LIFE,
    POWERUP_SPEED_BOOST,
    POWERUP_DOUBLE_DAMAGE
} PowerupType;

typedef struct {
    PowerupType type;
    SDL_Rect rect;
    bool active;
    bool picked_up;
    Uint32 pickup_time;
    Uint32 duration;
} Powerup;

#define MAX_POWERUPS 5

// Funktioner
Powerup create_powerup(PowerupType type, int x, int y);
void check_powerup_collision(Powerup* p, SDL_Rect player_rect, int* lives, int* player_speed, int* player_damage, Uint32 current_time);
void update_powerup_effect(Powerup* p, int* player_speed, int* player_damage, Uint32 current_time);
void draw_powerup(SDL_Renderer* renderer, Powerup* p);

extern SDL_Texture* tex_extralife;
extern SDL_Texture* tex_extraspeed;
extern SDL_Texture* tex_doubledamage;

#endif
