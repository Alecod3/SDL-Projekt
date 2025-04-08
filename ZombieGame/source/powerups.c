#include "powerups.h"

SDL_Texture* tex_extralife = NULL;
SDL_Texture* tex_extraspeed = NULL;
SDL_Texture* tex_doubledamage = NULL;

Powerup create_powerup(PowerupType type, int x, int y) {
    Powerup p;
    p.type = type;
    p.rect.x = x;
    p.rect.y = y;
    p.rect.w = 30;
    p.rect.h = 30;
    p.active = true;
    p.picked_up = false;
    p.pickup_time = 0;

    switch (type) {
        case POWERUP_SPEED_BOOST:
        case POWERUP_DOUBLE_DAMAGE:
            p.duration = 5000; // 5 sekunder
            break;
        default:
            p.duration = 0;
            break;
    }

    return p;
}

void check_powerup_collision(Powerup* p, SDL_Rect player_rect, int* lives, int* player_speed, int* player_damage, Uint32 current_time) {
    if (p->active && SDL_HasIntersection(&p->rect, &player_rect)) {
        p->active = false;
        p->picked_up = true;
        p->pickup_time = current_time;

        switch (p->type) {
            case POWERUP_EXTRA_LIFE:
                (*lives)++;
                break;
            case POWERUP_SPEED_BOOST:
                if (*player_speed == PLAYER_SPEED) {  // Skydda mot flera boostar
                    (*player_speed) *= 2;
                }
                break;
            case POWERUP_DOUBLE_DAMAGE:
                if (*player_damage == 1) {
                    (*player_damage) *= 2;
                }
                break;
        }
    }
}

void update_powerup_effect(Powerup* p, int* player_speed, int* player_damage, Uint32 current_time) {
    if (p->picked_up && p->duration > 0) {
        Uint32 elapsed = current_time - p->pickup_time;

        if (elapsed >= p->duration) {
            p->picked_up = false;

            switch (p->type) {
                case POWERUP_SPEED_BOOST:
                    if (*player_speed > PLAYER_SPEED) {
                        *player_speed = PLAYER_SPEED;
                    }
                    break;
                case POWERUP_DOUBLE_DAMAGE:
                    if (*player_damage > 1) {
                        *player_damage = 1;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void draw_powerup(SDL_Renderer* renderer, Powerup* p) {
    if (!p->active) return;

    SDL_Texture* tex = NULL;

    switch (p->type) {
        case POWERUP_EXTRA_LIFE:
            tex = tex_extralife;
            break;
        case POWERUP_SPEED_BOOST:
            tex = tex_extraspeed;
            break;
        case POWERUP_DOUBLE_DAMAGE:
            tex = tex_doubledamage;
            break;
    }

    if (tex != NULL) {
        SDL_RenderCopy(renderer, tex, NULL, &p->rect);
    }
}
