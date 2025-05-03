#include "mob.h"
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mob create_mob(int x, int y, int size, int type, int health)
{
    Mob m;
    m.rect.x = x;
    m.rect.y = y;

    switch (type)
    {
    case 0:
        m.rect.w = m.rect.h = 20;
        m.speed = 3.5f;
        break;
    case 1:
        m.rect.w = m.rect.h = 30;
        m.speed = 3.0f;
        break;
    case 2:
        m.rect.w = m.rect.h = 40;
        m.speed = 2.4f;
        break;
    case 3:
        m.rect.w = m.rect.h = 50;
        m.speed = 1.8f;
        break;
    default:
        m.rect.w = m.rect.h = size;
        m.speed = 3.0f;
        break;
    }

    m.active = true;
    m.type = type;
    m.health = health;

    m.attacking = false;
    m.last_attack_time = 0;
    m.attack_interval = 1000; // kan justeras

    return m;
}

bool check_mob_collision(SDL_Rect *mob_rect, Mob mobs[], int mob_index)
{
    for (int i = 0; i < MAX_MOBS; i++)
    {
        if (i != mob_index && mobs[i].active && SDL_HasIntersection(mob_rect, &mobs[i].rect))
        {
            return true;
        }
    }
    return false;
}

void update_mob(Mob *mob, SDL_Rect player_rect)
{
    if (!mob->active)
        return;

    // Om mobben når spelaren: gå in i attacking‐läge och sluta röra på dig
    if (SDL_HasIntersection(&mob->rect, &player_rect))
    {
        mob->attacking = true;
        return;
    }
    else
    {
        mob->attacking = false;
    }

    // Rörelse mot spelaren
    float dx = (player_rect.x + player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
    float dy = (player_rect.y + player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
    float len = sqrtf(dx * dx + dy * dy);
    if (len != 0)
    {
        dx /= len;
        dy /= len;

        // Lägg till lite slumpmässig offset för rörelsen
        dx += ((rand() % 100) - 50) / 100.0f * 0.3f;
        dy += ((rand() % 100) - 50) / 100.0f * 0.3f;
        len = sqrtf(dx * dx + dy * dy);
        dx /= len;
        dy /= len;

        int move_x = (int)(dx * mob->speed);
        int move_y = (int)(dy * mob->speed);
        if (move_x == 0 && dx != 0)
            move_x = dx > 0 ? 1 : -1;
        if (move_y == 0 && dy != 0)
            move_y = dy > 0 ? 1 : -1;

        SDL_Rect new_pos = mob->rect;
        new_pos.x += 0;
        new_pos.y += 0;

        // new_pos.x += move_x;
        // new_pos.y += move_y;

        if (new_pos.x >= 0 && new_pos.x + mob->rect.w <= SCREEN_WIDTH &&
            new_pos.y >= 0 && new_pos.y + mob->rect.h <= SCREEN_HEIGHT)
        {
            mob->rect = new_pos;
        }
        else
        {
            mob->rect.x += (rand() % 3 - 1) * (int)(mob->speed);
            mob->rect.y += (rand() % 3 - 1) * (int)(mob->speed);
        }
    }
}

extern SDL_Texture *tex_mob;

void draw_mob(SDL_Renderer *renderer, const Mob *mob, SDL_Rect player_rect)
{
    if (!mob->active)
        return;

    // Sätt färg beroende på typ (tint-färg)
    switch (mob->type)
    {
    case 0:
        SDL_SetTextureColorMod(tex_mob, 255, 0, 0); // Röd
        break;
    case 1:
        SDL_SetTextureColorMod(tex_mob, 0, 0, 255); // Blå
        break;
    case 2:
        SDL_SetTextureColorMod(tex_mob, 255, 165, 0); // Orange
        break;
    case 3:
        SDL_SetTextureColorMod(tex_mob, 128, 0, 128); // Lila
        break;
    default:
        SDL_SetTextureColorMod(tex_mob, 255, 255, 255); // Vit (neutral)
        break;
    }

    if (tex_mob)
    {
        // Beräkna riktning mot spelaren
        float dx = (player_rect.x + player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
        float dy = (player_rect.y + player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
        float angle = atan2f(dy, dx) * 180.0f / (float)M_PI;

        SDL_RenderCopyEx(renderer, tex_mob, NULL, &mob->rect, angle, NULL, SDL_FLIP_NONE);
    }
    else
    {
        // Fallback om tex_mob inte laddats
        switch (mob->type)
        {
        case 0:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case 1:
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            break;
        case 2:
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
            break;
        case 3:
            SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
            break;
        default:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            break;
        }
        SDL_RenderFillRect(renderer, &mob->rect);
    }
}