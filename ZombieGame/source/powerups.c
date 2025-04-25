#include "powerups.h"
#include <SDL2/SDL.h>
#include "player.h"
#include <stdbool.h>

Powerup create_powerup(PowerupType type, int x, int y)
{
    Powerup p;
    p.rect.x = x;
    p.rect.y = y;
    p.rect.w = POWERUP_SIZE;
    p.rect.h = POWERUP_SIZE;
    p.active = true;
    p.picked_up = false;
    p.type = type;
    p.sound_played = false;
    p.pickup_time = 0;
    switch (type)
    {
    case POWERUP_SPEED_BOOST:
        p.duration = POWERUP_SPEED_DURATION;
        break;
    case POWERUP_DOUBLE_DAMAGE:
        p.duration = POWERUP_DAMAGE_DURATION;
        break;
    case POWERUP_FREEZE_ENEMIES:
        p.duration = POWERUP_FREEZE_DURATION;
        break;
    default:
        p.duration = 0; // POWERUP_EXTRA_LIFE has no duration
        break;
    }
    return p;
}

void draw_powerup(SDL_Renderer *renderer, Powerup *p)
{
    if (p->active && !p->picked_up)
    {
        SDL_Texture *texture = NULL;
        switch (p->type)
        {
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
        if (texture)
        {
            SDL_RenderCopy(renderer, texture, NULL, &p->rect);
        }
    }
}

bool check_powerup_collision(Powerup *p, SDL_Rect player, int *lives, float *player_speed, int *player_damage, Uint32 current_time, ActiveEffects *effects)
{
    if (!p->active || p->picked_up)
        return false;
    if (SDL_HasIntersection(&player, &p->rect))
    {
        p->picked_up = true;
        p->pickup_time = current_time;
        switch (p->type)
        {
        case POWERUP_EXTRA_LIFE:
            if (*lives < MAX_HEALTH)
                (*lives)++;
            break;
        case POWERUP_SPEED_BOOST:
            if (!effects->speed_active)
                *player_speed *= 2.0f;
            effects->speed_active = true;
            effects->speed_start_time = current_time;
            break;
        case POWERUP_DOUBLE_DAMAGE:
            if (!effects->damage_active)
                *player_damage *= 2;
            effects->damage_active = true;
            effects->damage_start_time = current_time;
            break;
        case POWERUP_FREEZE_ENEMIES:
            if (!effects->freeze_active)
                effects->freeze_active = true;
            effects->freeze_start_time = current_time;
            break;
        }
        return true;
    }
    return false;
}

void apply_powerup_effect(Powerup *powerup, Player *player, ActiveEffects *effects, Uint32 now)
{
    switch (powerup->type)
    {
    case POWERUP_EXTRA_LIFE:
        if (player->lives < MAX_HEALTH)
        {
            player->lives++;
        }
        break;
    case POWERUP_SPEED_BOOST:
        if (!effects->speed_active)
            player->speed *= 2.0f;
        effects->speed_active = true;
        effects->speed_start_time = now;
        break;
    case POWERUP_DOUBLE_DAMAGE:
        if (!effects->speed_active)
            player->damage *= 2;
        effects->damage_active = true;
        effects->damage_start_time = now;
        break;
    case POWERUP_FREEZE_ENEMIES:
        effects->freeze_active = true;
        effects->freeze_start_time = now;
        break;
    }
    powerup->picked_up = true;
    powerup->pickup_time = now;
}
void update_effects(ActiveEffects *effects, float *player_speed, int *player_damage, Uint32 current_time, Powerup powerups[])
{
    if (effects->speed_active && (current_time - effects->speed_start_time) >= POWERUP_SPEED_DURATION)
    {
        *player_speed /= 2.0f;
        effects->speed_active = false;
    }
    if (effects->damage_active && (current_time - effects->damage_start_time) >= POWERUP_DAMAGE_DURATION)
    {
        *player_damage /= 2;
        effects->damage_active = false;
    }
    if (effects->freeze_active && (current_time - effects->freeze_start_time) >= POWERUP_FREEZE_DURATION)
    {
        effects->freeze_active = false;
    }
}

void draw_powerup_bars(SDL_Renderer *renderer, Player *player, Powerup powerups[], Uint32 current_time)
{
    int bar_y_offset = 10;
    int bar_width = player->rect.w;
    int bar_height = 5;

    if (player)
    {
        SDL_Rect bar_bg = {player->rect.x, player->rect.y - bar_y_offset, bar_width, bar_height};
        SDL_Rect bar_fg = {player->rect.x, player->rect.y - bar_y_offset, bar_width, bar_height};

        if (powerups[POWERUP_SPEED_BOOST].picked_up && powerups[POWERUP_SPEED_BOOST].pickup_time + powerups[POWERUP_SPEED_BOOST].duration > current_time)
        {
            Uint32 elapsed = current_time - powerups[POWERUP_SPEED_BOOST].pickup_time;
            int filled_width = (int)((1.0f - ((float)elapsed / powerups[POWERUP_SPEED_BOOST].duration)) * bar_width);
            bar_fg.w = filled_width;

            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &bar_bg);
            SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255);
            SDL_RenderFillRect(renderer, &bar_fg);
            bar_y_offset += bar_height + 2;
            bar_bg.y = player->rect.y - bar_y_offset;
            bar_fg.y = player->rect.y - bar_y_offset;
        }

        if (powerups[POWERUP_DOUBLE_DAMAGE].picked_up && powerups[POWERUP_DOUBLE_DAMAGE].pickup_time + powerups[POWERUP_DOUBLE_DAMAGE].duration > current_time)
        {
            Uint32 elapsed = current_time - powerups[POWERUP_DOUBLE_DAMAGE].pickup_time;
            int filled_width = (int)((1.0f - ((float)elapsed / powerups[POWERUP_DOUBLE_DAMAGE].duration)) * bar_width);
            bar_fg.w = filled_width;

            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &bar_bg);
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_RenderFillRect(renderer, &bar_fg);
            bar_y_offset += bar_height + 2;
            bar_bg.y = player->rect.y - bar_y_offset;
            bar_fg.y = player->rect.y - bar_y_offset;
        }

        if (powerups[POWERUP_FREEZE_ENEMIES].picked_up && powerups[POWERUP_FREEZE_ENEMIES].pickup_time + powerups[POWERUP_FREEZE_ENEMIES].duration > current_time)
        {
            Uint32 elapsed = current_time - powerups[POWERUP_FREEZE_ENEMIES].pickup_time;
            int filled_width = (int)((1.0f - ((float)elapsed / powerups[POWERUP_FREEZE_ENEMIES].duration)) * bar_width);
            bar_fg.w = filled_width;

            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &bar_bg);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderFillRect(renderer, &bar_fg);
        }
    }
}