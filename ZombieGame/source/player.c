#include "player.h"
#include <math.h>

Player create_player(int x, int y, int size, int speed, int damage, int lives) {
    Player p;
    p.rect.x = x;
    p.rect.y = y;
    p.rect.w = size;
    p.rect.h = size;
    p.speed = speed;
    p.damage = damage;
    p.lives = lives;
    return p;
}

void update_player(Player *p, const Uint8 *state) {
    int dx = 0, dy = 0;
    if ((state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W]) && p->rect.y > 0)
        dy = -1;
    if ((state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S]) && p->rect.y + p->rect.h < SCREEN_HEIGHT)
        dy = 1;
    if ((state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) && p->rect.x > 0)
        dx = -1;
    if ((state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) && p->rect.x + p->rect.w < SCREEN_WIDTH)
        dx = 1;
        
    float magnitude = sqrtf(dx * dx + dy * dy);
    if (magnitude != 0) {
        float norm_dx = (dx / magnitude) * p->speed;
        float norm_dy = (dy / magnitude) * p->speed;
        p->rect.x += (int)norm_dx;
        p->rect.y += (int)norm_dy;
    }
}

void draw_player(SDL_Renderer *renderer, const Player *p) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Grön färg
    SDL_RenderFillRect(renderer, &p->rect);
}
