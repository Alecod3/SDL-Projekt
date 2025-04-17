#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "player.h"
#include "mob.h"
#include "powerups.h"

// Skärm- och spelkonstanter
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 30
#define MOB_SIZE 30
#define MOB_SPEED 3
#define BULLET_SIZE 7
#define BULLET_SPEED 7
#define MAX_BULLETS 10
#define MAX_MOBS 5
#define MAX_POWERUPS 5
// HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH definieras exempelvis i powerups.h eller kan definieras här

#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20

// Bullet-ADT (här hålls den enkelt i main, men den kan även brytas ut)
typedef struct {
    SDL_Rect rect;
    bool active;
    float dx, dy;
} Bullet;

int main(int argc, char *argv[]) {
    // Initiera SDL2 och SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL kunde inte initieras: %s", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("SDL_image kunde inte initieras: %s", IMG_GetError());
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow("Zombie Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Fönster kunde inte skapas: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer kunde inte skapas: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Ladda in powerup-texturer (variablerna tex_extralife, etc. definieras i powerups.c som externa)
    tex_extralife    = IMG_LoadTexture(renderer, "resources/extralife.png");
    tex_extraspeed   = IMG_LoadTexture(renderer, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(renderer, "resources/doubledamage.png");
    tex_freezeenemies = IMG_LoadTexture(renderer, "resources/freezeenemies.png");

    if (!tex_extralife || !tex_extraspeed || !tex_doubledamage) {
        SDL_Log("Kunde inte ladda in powerup-texturer: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    srand((unsigned int)time(NULL));
    
    // Skapa spelaren via Player-ADT; DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE och MAX_HEALTH ska vara definierade (här antas de komma från powerups.h)
    Player player = create_player(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, PLAYER_SIZE, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    
    // Skapa bullets, mobs och powerups
    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS];
    for (int i = 0; i < MAX_MOBS; i++) {
        int x, y, type, health;
        do {
            x = rand() % (SCREEN_WIDTH - MOB_SIZE);
            y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
            type = rand() % 4; // Antag 4 typer
            health = (type == 3) ? 2 : 1;
            mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
        } while (check_mob_collision(&mobs[i].rect, mobs, i));
    }
    
    Powerup powerups[MAX_POWERUPS];
    for (int i = 0; i < MAX_POWERUPS; i++) {
        PowerupType t = rand() % 4;
        int x = rand() % (SCREEN_WIDTH - 32);
        int y = rand() % (SCREEN_HEIGHT - 32);
        powerups[i] = create_powerup(t, x, y);
    }
    
    int score = 0;

    Uint32 freeze_timer = 0;

    bool running = true;
    SDL_Event event;
    bool spacePressed = false;
    ActiveEffects effects = { false, 0, false, 0 };
    
    Uint32 now, last_mob_spawn_time = 0, last_powerup_spawn_time = 0;
    
    // Spel-loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                // Skjut en bullet med spelarens damage
                // (Skjutfunktionen finns i main, vi använder lokala bullets array)
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        bullets[i].rect.x = player.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.y = player.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;
                        float dx = (float)mx - (player.rect.x + PLAYER_SIZE / 2);
                        float dy = (float)my - (player.rect.y + PLAYER_SIZE / 2);
                        float length = sqrtf(dx * dx + dy * dy);
                        bullets[i].dx = BULLET_SPEED * (dx / length);
                        bullets[i].dy = BULLET_SPEED * (dy / length);
                        break;
                    }
                }
            }
        }
        
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        update_player(&player, state);
        
        if (state[SDL_SCANCODE_SPACE]) {
            if (!spacePressed) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        bullets[i].rect.x = player.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.y = player.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;
                        float dx = (float)mx - (player.rect.x + PLAYER_SIZE / 2);
                        float dy = (float)my - (player.rect.y + PLAYER_SIZE / 2);
                        float length = sqrtf(dx * dx + dy * dy);
                        bullets[i].dx = BULLET_SPEED * (dx / length);
                        bullets[i].dy = BULLET_SPEED * (dy / length);
                        break;
                    }
                }
                spacePressed = true;
            }
        } else {
            spacePressed = false;
        }
        
        // Kollision mellan spelare och mobs
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active && SDL_HasIntersection(&player.rect, &mobs[i].rect)) {
                player.lives--;
                mobs[i].active = false;
                if (player.lives <= 0) {
                    running = false;
                }
            }
        }
        
        // Uppdatera bullets: förflytta och kolla kollision med mobs
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].rect.x += (int)bullets[i].dx;
                bullets[i].rect.y += (int)bullets[i].dy;
                if (bullets[i].rect.x < 0 || bullets[i].rect.x > SCREEN_WIDTH ||
                    bullets[i].rect.y < 0 || bullets[i].rect.y > SCREEN_HEIGHT) {
                    bullets[i].active = false;
                    continue;
                }
                for (int j = 0; j < MAX_MOBS; j++) {
                    if (mobs[j].active && SDL_HasIntersection(&bullets[i].rect, &mobs[j].rect)) {
                        mobs[j].health -= player.damage;
                        if (mobs[j].health <= 0) {
                            mobs[j].active = false;
                            score += 100;
                            if (rand() % 4 == 0) { // 25% chans att en mob släpper en powerup vid död
                                for (int k = 0; k < MAX_POWERUPS; k++) {
                                    if (!powerups[k].active && !powerups[k].picked_up) {
                                        PowerupType t = rand() % 4;
                                        powerups[k] = create_powerup(t, mobs[j].rect.x, mobs[j].rect.y);
                                        break;
                                    }
                                }
                            }
                        }
                        bullets[i].active = false;
                        break;
                    }
                }
            }
        }
        
        // Uppdatera mobs så att de rör sig mot spelaren (om de inte är frysta)
        bool freeze_active = effects.freeze_active;
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                if (!freeze_active) {
                    update_mob(&mobs[i], player.rect);
                }
            }
        }
        
        // Spawna nya mobs var 1,5 sekund
        now = SDL_GetTicks();
        if (now - last_mob_spawn_time >= 1500) {
            for (int i = 0; i < MAX_MOBS; i++) {
                if (!mobs[i].active) {
                    int x, y, type, health;
                    do {
                        x = rand() % (SCREEN_WIDTH - MOB_SIZE);
                        y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
                        type = rand() % 4;
                        health = (type == 3) ? 2 : 1;
                        mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
                    } while (check_mob_collision(&mobs[i].rect, mobs, i));
                    break;
                }
            }
            last_mob_spawn_time = now;
        }
        
        // Spawna nya powerups var 5 sekund
        if (now - last_powerup_spawn_time >= 5000) {
            for (int i = 0; i < MAX_POWERUPS; i++) {
                if (!powerups[i].active) {
                    PowerupType t = rand() % 3;
                    int x = rand() % (SCREEN_WIDTH - 32);
                    int y = rand() % (SCREEN_HEIGHT - 32);
                    powerups[i] = create_powerup(t, x, y);
                    break;
                }
            }
            last_powerup_spawn_time = now;
        }
        
        // Hantera kollisioner och effekter för powerups
        for (int i = 0; i < MAX_POWERUPS; i++) {
            check_powerup_collision(&powerups[i], player.rect, &player.lives, &player.speed, &player.damage, now, &effects);
        }
        update_effects(&effects, &player.speed, &player.damage, now, powerups);
        
        // Renderingsfasen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        draw_player(renderer, &player);
        draw_powerup_bars(renderer, &player, powerups, now);
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                draw_mob(renderer, &mobs[i]);
            }
        }
        
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &bullets[i].rect);
            }
        }
        
        for (int i = 0; i < MAX_POWERUPS; i++) {
            draw_powerup(renderer, &powerups[i]);
        }
        
        // Rita en enkel livsbar
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect healthBarBackground = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBarBackground);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect healthBar = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, (HEALTH_BAR_WIDTH * player.lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBar);
        
        // Skriver ut score och lives på konsolen
        printf("Score: %d | Lives: %d\n", score, player.lives);
        SDL_RenderPresent(renderer);
        
        SDL_Delay(16);
    }
    
    // Rensa upp resurser
    SDL_DestroyTexture(tex_extralife);
    SDL_DestroyTexture(tex_extraspeed);
    SDL_DestroyTexture(tex_doubledamage);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}