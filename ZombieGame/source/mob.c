#include "mob.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Mob
{
    SDL_Rect rect;
    bool active;
    int type;
    int health;
    float speed;
    bool attacking;
    Uint32 last_attack_time;
    Uint32 attack_interval;
};

Mob *create_mob(int x, int y, int size, int type, int health)
{
    Mob *m = malloc(sizeof(Mob));
    m->rect.x = x;
    m->rect.y = y;

    switch (type)
    {
    case 0:
        m->rect.w = m->rect.h = 20;
        m->speed = 3.5f;
        break;
    case 1:
        m->rect.w = m->rect.h = 30;
        m->speed = 3.0f;
        break;
    case 2:
        m->rect.w = m->rect.h = 40;
        m->speed = 2.4f;
        break;
    case 3:
        m->rect.w = m->rect.h = 50;
        m->speed = 1.8f;
        break;
    default:
        m->rect.w = m->rect.h = size;
        m->speed = 3.0f;
        break;
    }

    m->active = true;
    m->type = type;
    m->health = health;
    m->attacking = false;
    m->last_attack_time = 0;
    m->attack_interval = 1000;
    return m;
}

void destroy_mob(Mob *m)
{
    free(m);
}

bool check_mob_collision(SDL_Rect *mob_rect, Mob **mobs, int mob_index)
{
    for (int i = 0; i < MAX_MOBS; i++)
    {
        if (i != mob_index && mob_is_active(mobs[i]) &&
            SDL_HasIntersection(mob_rect, &mobs[i]->rect))
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

    if (SDL_HasIntersection(&mob->rect, &player_rect))
    {
        mob->attacking = true;
        return;
    }
    else
    {
        mob->attacking = false;
    }

    float dx = (player_rect.x + player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
    float dy = (player_rect.y + player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
    float len = sqrtf(dx * dx + dy * dy);
    if (len != 0)
    {
        dx /= len;
        dy /= len;

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
        new_pos.x += move_x;
        new_pos.y += move_y;

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

void draw_mob(SDL_Renderer *renderer, const Mob *mob, SDL_Rect player_rect)
{
    if (!mob->active)
        return;

    switch (mob->type)
    {
    case 0:
        SDL_SetTextureColorMod(tex_mob, 255, 0, 0);
        break;
    case 1:
        SDL_SetTextureColorMod(tex_mob, 0, 0, 255);
        break;
    case 2:
        SDL_SetTextureColorMod(tex_mob, 255, 165, 0);
        break;
    case 3:
        SDL_SetTextureColorMod(tex_mob, 128, 0, 128);
        break;
    default:
        SDL_SetTextureColorMod(tex_mob, 255, 255, 255);
        break;
    }

    float dx = (player_rect.x + player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
    float dy = (player_rect.y + player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
    float angle = atan2f(dy, dx) * 180.0f / M_PI;

    SDL_RenderCopyEx(renderer, tex_mob, NULL, &mob->rect, angle, NULL, SDL_FLIP_NONE);
}

// Getters/setters

bool mob_is_attacking(const Mob *m)
{
    return m->attacking;
}

void mob_set_attacking(Mob *m, bool attacking)
{
    m->attacking = attacking;
}

Uint32 mob_get_last_attack_time(const Mob *m)
{
    return m->last_attack_time;
}

void mob_set_last_attack_time(Mob *m, Uint32 time)
{
    m->last_attack_time = time;
}

Uint32 mob_get_attack_interval(const Mob *m)
{
    return m->attack_interval;
}

SDL_Rect mob_get_rect(const Mob *m) { return m->rect; }
bool mob_is_active(const Mob *m) { return m->active; }
void mob_set_active(Mob *m, bool active) { m->active = active; }
int mob_get_type(const Mob *m) { return m->type; }
int mob_get_health(const Mob *m) { return m->health; }
void mob_set_health(Mob *m, int hp) { m->health = hp; }
