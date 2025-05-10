#ifndef MOB_H
#define MOB_H

#include <SDL2/SDL.h>
#include <stdbool.h>

#define MOB_SIZE 30
#define MOB_SPEED 3
#define MAX_MOBS 5
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct Mob Mob; // Opaque typ

// Skapande och förstöring
Mob *create_mob(int x, int y, int size, int type, int health);
void destroy_mob(Mob *m);

// Funktionalitet
bool check_mob_collision(SDL_Rect *mob_rect, Mob **mobs, int mob_index);
void update_mob(Mob *mob, SDL_Rect player_rect);
void draw_mob(SDL_Renderer *renderer, const Mob *mob, SDL_Rect player_rect);

// Getters / setters
SDL_Rect mob_get_rect(const Mob *m);
bool mob_is_active(const Mob *m);
void mob_set_active(Mob *m, bool active);
int mob_get_type(const Mob *m);
int mob_get_health(const Mob *m);
void mob_set_health(Mob *m, int hp);

bool mob_is_attacking(const Mob *m);
void mob_set_attacking(Mob *m, bool attacking);
Uint32 mob_get_last_attack_time(const Mob *m);
void mob_set_last_attack_time(Mob *m, Uint32 time);
Uint32 mob_get_attack_interval(const Mob *m);

extern SDL_Texture *tex_mob;

#endif
