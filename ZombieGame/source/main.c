#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "powerups.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 50
#define PLAYER_SPEED 5
#define MOB_SIZE 40
#define MOB_SPEED 2
#define BULLET_SIZE 10
#define BULLET_SPEED 7
#define MAX_BULLETS 10
#define MAX_MOBS 5
#define MAX_HEALTH 3
#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20

typedef struct {
    SDL_Rect rect;
    bool active;
} Bullet;

typedef struct {
    SDL_Rect rect;
    bool active;
} Mob;

bool check_mob_collision(SDL_Rect *mob, Mob *mobs, int mob_index) {
    for (int i = 0; i < MAX_MOBS; i++) {
        if (i != mob_index && mobs[i].active && SDL_HasIntersection(mob, &mobs[i].rect)) {
            return true;
        }
    }
    return false;
}

int SDL_main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        SDL_Log("SDL_image could not initialize! SDL_image Error: %s", IMG_GetError());
        return 1;
    }
    

    SDL_Window *window = SDL_CreateWindow("Simple SDL2 Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    tex_extralife    = IMG_LoadTexture(renderer, "resources/extralife.png");
    tex_extraspeed   = IMG_LoadTexture(renderer, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(renderer, "resources/doubledamage.png");

    if (!tex_extralife || !tex_extraspeed || !tex_doubledamage) {
        SDL_Log("Failed to load powerup textures: %s", SDL_GetError());
        return 1;
    }

    srand((unsigned int)time(NULL));
    SDL_Rect player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PLAYER_SIZE, PLAYER_SIZE};
    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS];
    int score = 0;
    int lives = MAX_HEALTH;

    int player_speed = PLAYER_SPEED;
    int player_damage = 1;
    Powerup powerups[MAX_POWERUPS];

    for (int i = 0; i < MAX_MOBS; i++) {
        do {
            mobs[i].rect.x = rand() % (SCREEN_WIDTH - MOB_SIZE);
            mobs[i].rect.y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
            mobs[i].rect.w = MOB_SIZE;
            mobs[i].rect.h = MOB_SIZE;
            mobs[i].active = true;
        } while (check_mob_collision(&mobs[i].rect, mobs, i));
    }

    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        PowerupType t = rand() % 3;
        int x = rand() % (SCREEN_WIDTH - 30);
        int y = rand() % (SCREEN_HEIGHT - 30);
        powerups[i] = create_powerup(t, x, y);
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        PowerupType t = rand() % 3;
        int x = rand() % (SCREEN_WIDTH - 30);
        int y = rand() % (SCREEN_HEIGHT - 30);
        powerups[i] = create_powerup(t, x, y);
    }
    

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_UP] && player.y > 0) player.y -= PLAYER_SPEED;
        if (state[SDL_SCANCODE_DOWN] && player.y + PLAYER_SIZE < SCREEN_HEIGHT) player.y += PLAYER_SPEED;
        if (state[SDL_SCANCODE_LEFT] && player.x > 0) player.x -= PLAYER_SPEED;
        if (state[SDL_SCANCODE_RIGHT] && player.x + PLAYER_SIZE < SCREEN_WIDTH) player.x += PLAYER_SPEED;

        if (state[SDL_SCANCODE_SPACE]) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].rect.x = player.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                    bullets[i].rect.y = player.y;
                    bullets[i].rect.w = BULLET_SIZE;
                    bullets[i].rect.h = BULLET_SIZE;
                    bullets[i].active = true;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active && SDL_HasIntersection(&player, &mobs[i].rect)) {
                lives--;
                mobs[i].active = false;
                if (lives <= 0) running = false;
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].rect.y -= BULLET_SPEED;
                if (bullets[i].rect.y < 0) bullets[i].active = false;
            }
        }

        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                SDL_Rect new_pos = mobs[i].rect;
                if (player.x < mobs[i].rect.x) new_pos.x -= MOB_SPEED;
                if (player.x > mobs[i].rect.x) new_pos.x += MOB_SPEED;
                if (player.y < mobs[i].rect.y) new_pos.y -= MOB_SPEED;
                if (player.y > mobs[i].rect.y) new_pos.y += MOB_SPEED;

                if (!check_mob_collision(&new_pos, mobs, i) && new_pos.x >= 0 && new_pos.x + MOB_SIZE <= SCREEN_WIDTH && new_pos.y >= 0 && new_pos.y + MOB_SIZE <= SCREEN_HEIGHT) {
                    mobs[i].rect = new_pos;
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        for (int i = 0; i < MAX_POWERUPS; i++) {
            check_powerup_collision(&powerups[i], player, &lives, &player_speed, &player_damage, now);
            update_powerup_effect(&powerups[i], &player_speed, &player_damage, now);
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &player);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                SDL_RenderFillRect(renderer, &mobs[i].rect);
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                SDL_RenderFillRect(renderer, &bullets[i].rect);
            }
        }

        for (int i = 0; i < MAX_POWERUPS; i++) {
            draw_powerup(renderer, &powerups[i]);
        }

        // Health bar rendering
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect healthBarBackground = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBarBackground);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect healthBar = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, (HEALTH_BAR_WIDTH * lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBar);

        // Print score and lives to console
        printf("Score: %d | Lives: %d\n", score, lives);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(tex_extralife);
    SDL_DestroyTexture(tex_extraspeed);
    SDL_DestroyTexture(tex_doubledamage);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
