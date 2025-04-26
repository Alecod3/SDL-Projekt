#include "powerups.h"
#include <SDL2/SDL.h>

// Externa texturer definieras här, de bör laddas (ex. med SDL_Image) någon annanstans i projektet.
SDL_Texture* tex_extralife = NULL;
SDL_Texture* tex_extraspeed = NULL;
SDL_Texture* tex_doubledamage = NULL;
SDL_Texture* tex_freezeenemies = NULL;
SDL_Texture* tex_ammo = NULL;

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
    if (type == POWERUP_SPEED_BOOST) {
        p.duration = 3000; // 3 sekunder i millisekunder
    } else if (type == POWERUP_DOUBLE_DAMAGE) {
        p.duration = 6000; // 6 sekunder i millisekunder
    } else if (POWERUP_EXTRA_LIFE) {
        p.duration = 0;
    } else if (type == POWERUP_FREEZE_ENEMIES) {
        p.duration = 3000; // 3 sekunder i millisekunder
    } else if (type == POWERUP_AMMO) {
        p.duration = 0;
    }
    p.sound_played = false;
    return p;
}

void check_powerup_collision(Powerup* p, SDL_Rect player, int* lives, int* player_speed, int* player_damage, int* player_ammo, Uint32 current_time, ActiveEffects* effects) {
    if (!p->active) return;

    SDL_Rect r = { p->rect.x, p->rect.y, p->rect.w, p->rect.h };
    if (SDL_HasIntersection(&r, &player)) {
        p->active = false;
        p->picked_up = true;
        p->pickup_time = current_time;

        switch (p->type) {
            case POWERUP_EXTRA_LIFE:
                *lives = (*lives < MAX_HEALTH) ? *lives + 1 : *lives;
                break;
            case POWERUP_AMMO:
                *player_ammo += 10;
                if (*player_ammo > MAX_AMMO)
                    *player_ammo = MAX_AMMO;
                break;
            case POWERUP_SPEED_BOOST:
                *player_speed = DEFAULT_PLAYER_SPEED * 2;
                effects->speed_active = true;
                effects->speed_start_time = current_time;
                break;
            case POWERUP_DOUBLE_DAMAGE:
                *player_damage = DEFAULT_PLAYER_DAMAGE * 2;
                effects->damage_active = true;
                effects->damage_start_time = current_time;
                break;
            case POWERUP_FREEZE_ENEMIES:
                effects->freeze_active = true;
                effects->freeze_start_time = current_time;
                break;
        }
    }
}

void update_effects(ActiveEffects* effects, int* player_speed, int* player_damage, Uint32 current_time, Powerup powerups[]) {
    if (effects->speed_active && current_time - effects->speed_start_time >= 3000) {
        *player_speed = DEFAULT_PLAYER_SPEED;
        effects->speed_active = false;
    }

    if (effects->damage_active && current_time - effects->damage_start_time >= 3000) {
        *player_damage = DEFAULT_PLAYER_DAMAGE;
        effects->damage_active = false;
    }

    if (effects->freeze_active && current_time - effects->freeze_start_time >= 3000) {
        effects->freeze_active = false;
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (powerups[i].picked_up && powerups[i].duration > 0 &&
            current_time - powerups[i].pickup_time >= powerups[i].duration) {
            powerups[i].picked_up = false;
        }
    }
}


void update_powerup_effect(Powerup* powerups, int index, int* player_speed, int* player_damage, Uint32 current_time) {
    Powerup* p = &powerups[index];

    if (!p->picked_up || p->duration == 0) return;

    if (current_time - p->pickup_time >= p->duration) {
        p->picked_up = false;

        // Kolla om någon annan powerup av samma typ fortfarande är aktiv
        bool same_type_active = false;
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (i != index && powerups[i].picked_up && powerups[i].type == p->type) {
                same_type_active = true;
                break;
            }
        }

        // Om ingen annan likadan powerup är aktiv, återställ attribut
        if (!same_type_active) {
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
        }
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
        case POWERUP_FREEZE_ENEMIES:
            texture = tex_freezeenemies;
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
