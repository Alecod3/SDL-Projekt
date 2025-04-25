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
    m.attack_interval = 1000; // 1 second attack interval

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

void update_mob(Mob *mob, SDL_Rect local_player_rect, SDL_Rect remote_player_rect, bool is_multiplayer)
{
    if (!mob->active)
        return;

    // Determine the target player (closest in multiplayer, local in single-player)
    SDL_Rect target_rect = local_player_rect;
    if (is_multiplayer)
    {
        float dx_local = (local_player_rect.x + local_player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
        float dy_local = (local_player_rect.y + local_player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
        float dist_local = sqrtf(dx_local * dx_local + dy_local * dy_local);

        float dx_remote = (remote_player_rect.x + remote_player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
        float dy_remote = (remote_player_rect.y + remote_player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
        float dist_remote = sqrtf(dx_remote * dx_remote + dy_remote * dy_remote);

        if (dist_remote < dist_local)
        {
            target_rect = remote_player_rect;
        }
    }

    // Check if mob reaches the target player
    if (SDL_HasIntersection(&mob->rect, &target_rect))
    {
        mob->attacking = true;
        return;
    }
    else
    {
        mob->attacking = false;
    }

    // Move toward the target player
    float dx = (target_rect.x + target_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
    float dy = (target_rect.y + target_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
    float len = sqrtf(dx * dx + dy * dy);
    if (len != 0)
    {
        dx /= len;
        dy /= len;

        // Add slight random offset for varied movement
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

        // Ensure mob stays within screen bounds
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

    // Set color tint based on mob type
    switch (mob->type)
    {
    case 0:
        SDL_SetTextureColorMod(tex_mob, 255, 0, 0); // Red
        break;
    case 1:
        SDL_SetTextureColorMod(tex_mob, 0, 0, 255); // Blue
        break;
    case 2:
        SDL_SetTextureColorMod(tex_mob, 255, 165, 0); // Orange
        break;
    case 3:
        SDL_SetTextureColorMod(tex_mob, 128, 0, 128); // Purple
        break;
    default:
        SDL_SetTextureColorMod(tex_mob, 255, 255, 255); // White
        break;
    }

    if (tex_mob)
    {
        // Calculate angle toward player
        float dx = (player_rect.x + player_rect.w / 2) - (mob->rect.x + mob->rect.w / 2);
        float dy = (player_rect.y + player_rect.h / 2) - (mob->rect.y + mob->rect.h / 2);
        float angle = atan2f(dy, dx) * 180.0f / (float)M_PI;

        SDL_RenderCopyEx(renderer, tex_mob, NULL, &mob->rect, angle, NULL, SDL_FLIP_NONE);
    }
    else
    {
        // Fallback rendering if texture is missing
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