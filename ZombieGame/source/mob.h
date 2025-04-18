#ifndef MOB_H
#define MOB_H

#include <SDL2/SDL.h>
#include <stdbool.h>

// Definiera konstanter
#define MOB_SIZE 30
#define MOB_SPEED 3
#define MAX_MOBS 5
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// ADT för fiender (mobs)
typedef struct {
    SDL_Rect rect;
    bool active;
    int type;    // Används exempelvis för att avgöra färg eller om fienden har extra HP
    int health;
    float speed;
} Mob;

// Funktioner för att hantera mobs
Mob create_mob(int x, int y, int size, int type, int health);
bool check_mob_collision(SDL_Rect *mob_rect, Mob mobs[], int mob_index);
void update_mob(Mob *mob, SDL_Rect player_rect);
void draw_mob(SDL_Renderer *renderer, const Mob *mob, SDL_Rect player_rect);

extern SDL_Texture* tex_mob;

#endif // MOB_H

