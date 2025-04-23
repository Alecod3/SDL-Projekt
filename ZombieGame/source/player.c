#include "player.h"
#include <math.h>
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern SDL_Texture *tex_player;

Player create_player(int x, int y, int size, float speed, int damage, int lives)
{
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

void update_player(Player *player, const Uint8 *state)
{
    int dx = 0, dy = 0;
    if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP])
        dy -= 1;
    if (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN])
        dy += 1;
    if (state[SDL_SCANCODE_A] || state[SDL_SCANCODE_LEFT])
        dx -= 1;
    if (state[SDL_SCANCODE_D] || state[SDL_SCANCODE_RIGHT])
        dx += 1;

    if (dx != 0 || dy != 0)
    {
        float length = sqrtf(dx * dx + dy * dy);
        dx = (int)(player->speed * dx / length);
        dy = (int)(player->speed * dy / length);
    }

    player->rect.x += dx;
    player->rect.y += dy;

    if (player->rect.x < 0)
        player->rect.x = 0;
    if (player->rect.y < 0)
        player->rect.y = 0;
    if (player->rect.x > SCREEN_WIDTH - player->rect.w)
        player->rect.x = SCREEN_WIDTH - player->rect.w;
    if (player->rect.y > SCREEN_HEIGHT - player->rect.h)
        player->rect.y = SCREEN_HEIGHT - player->rect.h;
}

void draw_player(SDL_Renderer *renderer, Player *player)
{
    float angle;
    if (player->angle != 0.0f)
    {
        // Use stored angle for remote player
        angle = player->angle;
    }
    else
    {
        // Calculate angle based on mouse for local player
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        float dx = (float)mx - (player->rect.x + player->rect.w / 2);
        float dy = (float)my - (player->rect.y + player->rect.h / 2);
        angle = atan2f(dy, dx) * 180.0f / M_PI;
    }

    SDL_Rect dest = {player->rect.x, player->rect.y, player->rect.w, player->rect.h};
    SDL_RenderCopyEx(renderer, tex_player, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
}