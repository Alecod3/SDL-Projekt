#include "mob.h"
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

Mob create_mob(int x, int y, int size, int type, int health) {
    Mob m;
    m.rect.x = x;
    m.rect.y = y;
    m.rect.w = size;
    m.rect.h = size;
    m.active = true;
    m.type = type;
    m.health = health;
    return m;
}

bool check_mob_collision(SDL_Rect *mob_rect, Mob mobs[], int mob_index) {
    for (int i = 0; i < MAX_MOBS; i++) {
        if (i != mob_index && mobs[i].active && SDL_HasIntersection(mob_rect, &mobs[i].rect)) {
            return true;
        }
    }
    return false;
}

void update_mob(Mob *mob, SDL_Rect player_rect) {
    if (!mob->active)
        return;
    
    float dx = player_rect.x - mob->rect.x;
    float dy = player_rect.y - mob->rect.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length != 0) {
        dx /= length;
        dy /= length;
        // Litet slumpmässigt offset
        float offset_x = ((rand() % 100) - 50) / 100.0f * 0.3f;
        float offset_y = ((rand() % 100) - 50) / 100.0f * 0.3f;
        dx += offset_x;
        dy += offset_y;
        length = sqrtf(dx * dx + dy * dy);
        dx /= length;
        dy /= length;
        
        SDL_Rect new_pos = mob->rect;
        new_pos.x += (int)(dx * MOB_SPEED);
        new_pos.y += (int)(dy * MOB_SPEED);
        
        if (new_pos.x >= 0 && new_pos.x + MOB_SIZE <= SCREEN_WIDTH &&
            new_pos.y >= 0 && new_pos.y + MOB_SIZE <= SCREEN_HEIGHT) {
            mob->rect = new_pos;
        } else {
            // Alternativ kort slumpmässig rörelse vid kollision
            mob->rect.x += (rand() % 3 - 1) * MOB_SPEED;
            mob->rect.y += (rand() % 3 - 1) * MOB_SPEED;
        }
    }
}

void draw_mob(SDL_Renderer *renderer, const Mob *mob) {
    if (!mob->active)
        return;
    
    switch (mob->type) {
        case 0:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);   // Röd
            break;
        case 1:
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);   // Blå
            break;
        case 2:
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Orange
            break;
        case 3:
            SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255); // Lila (2 HP)
            break;
        default:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            break;
    }
    SDL_RenderFillRect(renderer, &mob->rect);
}
