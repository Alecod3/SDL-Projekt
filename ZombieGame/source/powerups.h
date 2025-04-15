#ifndef POWERUPS_H
#define POWERUPS_H
#define PLAYER_SPEED 3
#define DEFAULT_PLAYER_SPEED 5
#define DEFAULT_PLAYER_DAMAGE 1
#define MAX_HEALTH 3
#define MAX_POWERUPS 5

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum {
    POWERUP_EXTRA_LIFE,
    POWERUP_SPEED_BOOST,
    POWERUP_DOUBLE_DAMAGE
} PowerupType;

typedef struct {
    bool speed_active;
    Uint32 speed_start_time;
    bool damage_active;
    Uint32 damage_start_time;
} ActiveEffects;

typedef struct {
    PowerupType type;
    SDL_Rect rect;
    bool active;
    bool picked_up;
    Uint32 pickup_time;
    Uint32 duration;
} Powerup;

// Funktioner
Powerup create_powerup(PowerupType type, int x, int y);
void check_powerup_collision(Powerup* p, SDL_Rect player_rect, int* lives, int* player_speed, int* player_damage, Uint32 current_time, ActiveEffects* effects);
void update_powerup_effect(Powerup* powerups, int index, int* player_speed, int* player_damage, Uint32 current_time);
void draw_powerup(SDL_Renderer* renderer, Powerup* p);
void update_effects(ActiveEffects* effects, int* player_speed, int* player_damage, Uint32 current_time);

extern SDL_Texture* tex_extralife;
extern SDL_Texture* tex_extraspeed;
extern SDL_Texture* tex_doubledamage;

#endif
