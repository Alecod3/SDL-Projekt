#ifndef POWERUPS_H
#define POWERUPS_H
#include <SDL2/SDL.h>
#include "sound.h"
#include "player.h"

#define MAX_POWERUPS 5
#define POWERUP_SIZE 32
#define POWERUP_SPEED_DURATION 5000
#define POWERUP_DAMAGE_DURATION 5000
#define POWERUP_FREEZE_DURATION 5000

typedef enum
{
    POWERUP_EXTRA_LIFE,
    POWERUP_SPEED_BOOST,
    POWERUP_DOUBLE_DAMAGE,
    POWERUP_FREEZE_ENEMIES
} PowerupType;

typedef struct
{
    SDL_Rect rect;
    bool active;
    bool picked_up;
    PowerupType type;
    bool sound_played;
    Uint32 duration;
    Uint32 pickup_time;
} Powerup;

typedef struct
{
    bool speed_active;
    bool damage_active;
    bool freeze_active;
    Uint32 speed_start_time;
    Uint32 damage_start_time;
    Uint32 freeze_start_time;
} ActiveEffects;

extern SDL_Texture *tex_extralife;
extern SDL_Texture *tex_extraspeed;
extern SDL_Texture *tex_doubledamage;
extern SDL_Texture *tex_freezeenemies;

Powerup create_powerup(PowerupType type, int x, int y);
void draw_powerup(SDL_Renderer *renderer, Powerup *p);
bool check_powerup_collision(Powerup *p, SDL_Rect player, int *lives, float *player_speed, int *player_damage, Uint32 current_time, ActiveEffects *effects);
void update_effects(ActiveEffects *effects, float *player_speed, int *player_damage, Uint32 current_time, Powerup powerups[]);
void draw_powerup_bars(SDL_Renderer *renderer, Player *player, Powerup powerups[], Uint32 current_time);

#endif