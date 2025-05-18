#include "player.h"
#include <math.h>
#include "powerups.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Player
{
    SDL_Rect rect;
    int speed;
    int damage;
    int lives;
    float aim_angle;
    SDL_Color tint;

    int ammo;
    bool reloading;
    Uint32 reload_start_time;
};

Player *create_player(int x, int y, int size, int speed, int damage, int lives)
{
    Player *p = malloc(sizeof(Player));
    p->rect.x = x;
    p->rect.y = y;
    p->rect.w = size;
    p->rect.h = size;
    p->speed = speed;
    p->damage = damage;
    p->lives = lives;
    p->aim_angle = 0.0f;
    p->tint = (SDL_Color){255, 255, 255, 255};

    p->ammo = 30;
    p->reloading = false;
    p->reload_start_time = 0;
    return p;
}

Uint32 player_get_reload_start_time(const Player *p)
{
    return p->reload_start_time;
}

SDL_Rect player_get_rect(const Player *p)
{
    return p->rect;
}

void network_send_set_max_mobs(int max_mobs)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_SET_MAX_MOBS;
    memcpy(d + 1, &max_mobs, sizeof(int));
    pktOut->len = 1 + sizeof(int);
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

int player_get_ammo(const Player *p)
{
    return p->ammo;
}

void player_set_ammo(Player *p, int ammo)
{
    p->ammo = ammo;
}

bool player_is_reloading(const Player *p)
{
    return p->reloading;
}

void player_start_reload(Player *p, Uint32 now)
{
    p->reloading = true;
    p->reload_start_time = now;
}

void player_finish_reload(Player *p)
{
    p->reloading = false;
    p->ammo = 15;
}

int player_get_lives(const Player *p)
{
    return p->lives;
}

void player_set_lives(Player *p, int lives)
{
    p->lives = lives;
}

void player_set_tint(Player *p, SDL_Color tint)
{
    p->tint = tint;
}

int player_get_x(const Player *p)
{
    return p->rect.x;
}

int player_get_y(const Player *p)
{
    return p->rect.y;
}
void player_set_aim_angle(Player *p, float angle)
{
    p->aim_angle = angle;
}

float player_get_aim_angle(const Player *p)
{
    return p->aim_angle;
}

int player_get_center_x(const Player *p)
{
    return p->rect.x + p->rect.w / 2;
}

int player_get_center_y(const Player *p)
{
    return p->rect.y + p->rect.h / 2;
}

void player_set_position(Player *p, int x, int y)
{
    p->rect.x = x;
    p->rect.y = y;
}

int player_get_speed(const Player *p)
{
    return p->speed;
}

void player_set_speed(Player *p, int speed)
{
    p->speed = speed;
}

int player_get_damage(const Player *p)
{
    return p->damage;
}

void player_set_damage(Player *p, int damage)
{
    p->damage = damage;
}

void update_player(Player *p, const Uint8 *state)
{
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
    if (magnitude != 0)
    {
        float norm_dx = (dx / magnitude) * p->speed;
        float norm_dy = (dy / magnitude) * p->speed;
        p->rect.x += (int)norm_dx;
        p->rect.y += (int)norm_dy;
    }
}

extern SDL_Texture *tex_player;

extern SDL_Texture *tex_player;

void draw_player(SDL_Renderer *renderer, const Player *p)
{
    if (tex_player)
    {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // Spelarens mittpunkt
        int px = p->rect.x + p->rect.w / 2;
        int py = p->rect.y + p->rect.h / 2;

        // Vektor till musen
        float dx = mx - px;
        float dy = my - py;

        // Räkna ut vinkel i grader
        float angle = atan2f(dy, dx) * (180.0f / M_PI);

        SDL_Point center = {p->rect.w / 2, p->rect.h / 2};

        SDL_RenderCopyEx(renderer, tex_player, NULL, &p->rect, angle, &center, SDL_FLIP_NONE);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // fallback-färg
        SDL_RenderFillRect(renderer, &p->rect);
    }
}

void draw_powerup_bars(SDL_Renderer *renderer, const Player *p, Powerup powerups[], int now)
{
    int bar_y_offset = 10;
    int bar_width = p->rect.w;
    int bar_height = 5;

    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerups[i].picked_up || powerups[i].duration == 0 || powerups[i].type == POWERUP_EXTRA_LIFE)
            continue;
        Uint32 elapsed = now - powerups[i].pickup_time;
        if (elapsed >= powerups[i].duration)
        {
            powerups[i].picked_up = false;
            continue;
        }
        int filled_width = (int)((1.0f - ((float)elapsed / powerups[i].duration)) * bar_width);
        SDL_Rect bar_bg = {p->rect.x, p->rect.y - bar_y_offset, bar_width, bar_height};
        SDL_Rect bar_fg = {p->rect.x, p->rect.y - bar_y_offset, filled_width, bar_height};

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Mörkgrå bakgrund
        SDL_RenderFillRect(renderer, &bar_bg);

        switch (powerups[i].type)
        {
        case POWERUP_SPEED_BOOST:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Gul
            break;
        case POWERUP_DOUBLE_DAMAGE:
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Grå
            break;
        case POWERUP_FREEZE_ENEMIES:
            SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255); // Blå
            break;
        default:
            break;
        }
        SDL_RenderFillRect(renderer, &bar_fg);
        bar_y_offset += bar_height + 2;
    }
}
