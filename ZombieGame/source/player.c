#include "player.h"
#include <math.h>
#include "powerups.h"

Player create_player(int x, int y, int size, int speed, int damage, int lives) {
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

void update_player(Player *p, const Uint8 *state) {
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
    if (magnitude != 0) {
        float norm_dx = (dx / magnitude) * p->speed;
        float norm_dy = (dy / magnitude) * p->speed;
        p->rect.x += (int)norm_dx;
        p->rect.y += (int)norm_dy;
    }
}

extern SDL_Texture* tex_player;

void draw_player(SDL_Renderer *renderer, const Player *p) {
    if (tex_player) {
        SDL_RenderCopy(renderer, tex_player, NULL, &p->rect);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // fallback-färg
        SDL_RenderFillRect(renderer, &p->rect);
    }
}

void draw_powerup_bars(SDL_Renderer *renderer, const Player *p, Powerup powerups[], int now) {
    int bar_y_offset = 10;
    int bar_width = p->rect.w;
    int bar_height = 5;

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].picked_up || powerups[i].duration == 0 || powerups[i].type == POWERUP_EXTRA_LIFE)
            continue;
        Uint32 elapsed = now - powerups[i].pickup_time;
        if (elapsed >= powerups[i].duration) {
            powerups[i].picked_up = false;
            continue;
        }
        int filled_width = (int)((1.0f - ((float)elapsed / powerups[i].duration)) * bar_width);
        SDL_Rect bar_bg = {p->rect.x, p->rect.y - bar_y_offset, bar_width, bar_height};
        SDL_Rect bar_fg = {p->rect.x, p->rect.y - bar_y_offset, filled_width, bar_height};

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Mörkgrå bakgrund
        SDL_RenderFillRect(renderer, &bar_bg);

        switch (powerups[i].type) {
            case POWERUP_SPEED_BOOST:
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Gul
                break;
            case POWERUP_DOUBLE_DAMAGE:
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Grå
                break;
            case POWERUP_FREEZE_ENEMIES:
                SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255); // Blå
                break;
            }
        SDL_RenderFillRect(renderer, &bar_fg);
        bar_y_offset += bar_height + 2;
    }
}
