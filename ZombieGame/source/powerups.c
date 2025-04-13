#include "powerups.h"
#include <SDL2/SDL.h>

// Externa texturer definieras här, de bör laddas (ex. med SDL_Image) någon annanstans i projektet.
SDL_Texture* tex_extralife = NULL;
SDL_Texture* tex_extraspeed = NULL;
SDL_Texture* tex_doubledamage = NULL;

// Skapar en ny powerup med position och typ
Powerup create_powerup(PowerupType type, int x, int y) {
    Powerup p;
    p.type = type;
    p.rect.x = x;
    p.rect.y = y;
    p.rect.w = 32; // Exempelstorlek
    p.rect.h = 32; // Exempelstorlek
    p.active = true;
    p.picked_up = false;
    p.pickup_time = 0;
    // Sätter duration (10 sekunder) för powerups som har tidsbegränsad effekt
    if (type == POWERUP_SPEED_BOOST || type == POWERUP_DOUBLE_DAMAGE) {
        p.duration = 10000; // 10 sekunder i millisekunder
    } else {
        p.duration = 0; // Ingen duration för extra life
    }
    return p;
}

void check_powerup_collision(Powerup* p, SDL_Rect player_rect, int* lives, int* player_speed, int* player_damage, Uint32 current_time) {
    // Om powerup:en är inaktiv eller redan plockad, gör inget
    if (!p->active || p->picked_up) return;

    if (SDL_HasIntersection(&p->rect, &player_rect)) {
        switch (p->type) {
            case POWERUP_EXTRA_LIFE:
            if(*lives < MAX_HEALTH){
                (*lives)++;    
            }
                break;
            case POWERUP_SPEED_BOOST:
                *player_speed = PLAYER_SPEED * 3;  // trippla hastigheten
                break;
            case POWERUP_DOUBLE_DAMAGE:
                *player_damage = (*player_damage) * 2; // Dubbla skadan
                break;
        }
        // Markera att powerup:en plockats upp och gör den inaktiv så att den inte triggar om sig
        p->picked_up = true;
        p->active = false;  
        p->pickup_time = current_time;
    }
}

void update_powerup_effect(Powerup* p, int* player_speed, int* player_damage, Uint32 current_time) {
    // Om ingen powerup-effekt är aktiv, returnera
    if (!p->picked_up) return;

    // Om durationen har gått ut, återställ spelarens attribut
    if (current_time - p->pickup_time >= p->duration) {
        switch (p->type) {
            case POWERUP_SPEED_BOOST:
                *player_speed = DEFAULT_PLAYER_SPEED;
                break;
            case POWERUP_DOUBLE_DAMAGE:
                *player_damage = DEFAULT_PLAYER_DAMAGE;
                break;
            default:
                break;
        }
        // Flagga återställs så att den inte kan trigga en återställning igen
        p->picked_up = false;
    }
}


// Renderar powerup på skärmen
void draw_powerup(SDL_Renderer* renderer, Powerup* p) {
    if (!p->active) return;

    SDL_Texture* texture = NULL;
    switch (p->type) {
        case POWERUP_EXTRA_LIFE:
            texture = tex_extralife;
            break;
        case POWERUP_SPEED_BOOST:
            texture = tex_extraspeed;
            break;
        case POWERUP_DOUBLE_DAMAGE:
            texture = tex_doubledamage;
            break;
    }

    if (texture != NULL) {
        SDL_RenderCopy(renderer, texture, NULL, &p->rect);
    } else {
        // Om ingen textur är laddad, rita en röd rektangel som en placeholder
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &p->rect);
    }
}
