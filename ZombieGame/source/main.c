#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "player.h"
#include "mob.h"
#include "powerups.h"
#include "sound.h"

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

bool skipMenu = false;
SDL_Texture* tex_player = NULL;
SDL_Texture* tex_mob = NULL;
SDL_Texture* tex_tiles = NULL;

int showSettings(SDL_Renderer *renderer, SDL_Window *window);

// Bullet-ADT (här hålls den enkelt i main, men den kan även brytas ut)
typedef struct {
    SDL_Rect rect;
    bool active;
    float dx, dy;
} Bullet;

int showMenu(SDL_Renderer *renderer, SDL_Window *window) {
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char* labels[3] = {"Single Player", "Multiplayer", "Settings"};

    TTF_Font* font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font* titleFont = TTF_OpenFont("simbiot.ttf", 64);

    for (int i = 0; i < 3; i++) {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }

    while (inMenu) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return -1;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) selected = (selected + 2) % 3;
                if (event.key.keysym.sym == SDLK_DOWN) selected = (selected + 1) % 3;
                if (event.key.keysym.sym == SDLK_RETURN) {
                    if (selected == 2) { // Settings
                        showSettings(renderer, window);
                    } else {
                        return selected;
                    }
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 3; i++) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) return i;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont) {
            SDL_Color red = {200, 0, 0};
            SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, "Zombie Game", red);
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = { SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h };
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }

        for (int i = 0; i < 3; i++) {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color color = {255, 255, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, labels[i], color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(titleFont);
    TTF_CloseFont(font);
    return 0;
}

int showSettings(SDL_Renderer *renderer, SDL_Window *window) {
    SDL_Event event;
    bool inSettings = true;
    TTF_Font* font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font* titleFont = TTF_OpenFont("simbiot.ttf", 48);

    if (!font || !titleFont) {
        SDL_Log("Misslyckades med att ladda font i inställningsmeny: %s", TTF_GetError());
        return 0;
    }

    while (inSettings) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return -1;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN) {
                    inSettings = false;  // Gå tillbaka till menyn
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // --- Rubrik: Settings ---
        SDL_Color white = {255, 255, 255};
        SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, "Settings", white);
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Rect titleRect = { SCREEN_WIDTH / 2 - titleSurface->w / 2, 50, titleSurface->w, titleSurface->h };
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_FreeSurface(titleSurface);
        SDL_DestroyTexture(titleTexture);

        // --- Volym-rubrik till vänster ---
        SDL_Surface* volumeSurface = TTF_RenderText_Blended(font, "Volume", white);
        SDL_Texture* volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
        SDL_Rect volumeRect = { 100, 180, volumeSurface->w, volumeSurface->h };
        SDL_RenderCopy(renderer, volumeTexture, NULL, &volumeRect);
        SDL_FreeSurface(volumeSurface);
        SDL_DestroyTexture(volumeTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    TTF_CloseFont(titleFont);
    return 0;
}

int showGameOver(SDL_Renderer *renderer) {
    SDL_Event event;
    bool inGameOver = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char* labels[3] = {"Play Again", "Main Menu", "Exit Game"};

    TTF_Font* optionFont = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font* titleFont = TTF_OpenFont("simbiot.ttf", 64);

    for (int i = 0; i < 3; i++) {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }

    while (inGameOver) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return 2;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) selected = (selected + 2) % 3;
                if (event.key.keysym.sym == SDLK_DOWN) selected = (selected + 1) % 3;
                if (event.key.keysym.sym == SDLK_RETURN) return selected;
            }
            if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 3; i++) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) return i;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont) {
            SDL_Color red = {200, 0, 0};
            SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, "GAME OVER", red);
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = { SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h };
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }

        for (int i = 0; i < 3; i++) {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color color = {255, 255, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(optionFont, labels[i], color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(optionFont);
    TTF_CloseFont(titleFont);
    return 0;
}

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

    if (TTF_Init() == -1) {
        SDL_Log("SDL_ttf init error: %s", TTF_GetError());
        SDL_Quit();
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

    init_sound();
    play_music("source/spelmusik.wav");
    
    if (!skipMenu) {
        int menuResult = showMenu(renderer, window);
        if (menuResult != 0) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 0;
        }
    }
    
    
    // Ladda in powerup-texturer (variablerna tex_extralife, etc. definieras i powerups.c som externa)
    tex_extralife    = IMG_LoadTexture(renderer, "resources/extralife.png");
    tex_extraspeed   = IMG_LoadTexture(renderer, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(renderer, "resources/doubledamage.png");
    tex_freezeenemies = IMG_LoadTexture(renderer, "resources/freezeenemies.png");
    tex_ammo = IMG_LoadTexture(renderer, "resources/ammo.png");

    tex_tiles = IMG_LoadTexture(renderer, "resources/tiles.png");
if (!tex_tiles) {
    SDL_Log("Kunde inte ladda in tilesheet: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
}

    tex_player = IMG_LoadTexture(renderer, "resources/hitman1.png");
if (!tex_player) {
    SDL_Log("Kunde inte ladda in spelartextur: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
}

tex_mob = IMG_LoadTexture(renderer, "resources/zombie1.png");
if (!tex_mob) {
    SDL_Log("Kunde inte ladda in fiendetextur: %s", SDL_GetError());
    SDL_DestroyTexture(tex_player); // du har laddat spelartestur innan
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
}

    if (!tex_extralife || !tex_ammo || !tex_extraspeed || !tex_doubledamage || !tex_freezeenemies) {
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
        PowerupType t = rand() % 5;
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
                        play_sound(SOUND_SHOOT);
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
                        play_sound(SOUND_SHOOT);
                        break;
                    }
                }
                spacePressed = true;
            }
        } else {
            spacePressed = false;
        }
        
        // Kollision och attack‑hantering mellan spelare och mobs
for (int i = 0; i < MAX_MOBS; i++) {
    if (!mobs[i].active)
        continue;

    if (mobs[i].attacking) {
        Uint32 now = SDL_GetTicks();
        // Slå bara om attack_interval passerat
        if (now - mobs[i].last_attack_time >= mobs[i].attack_interval) {
            if (mobs[i].type == 3) {
                player.lives -= 2;
            } else {
                player.lives--;
            }
            mobs[i].last_attack_time = now;

            // Kolla om spelaren dör
            if (player.lives <= 0) {
                int result = showGameOver(renderer);
                if (result == 0) {
                    skipMenu = true;
                    return main(argc, argv);
                } else if (result == 1) {
                    skipMenu = false;
                    return main(argc, argv);
                } else {
                    running = false;
                }
            }
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
                                        PowerupType t = rand() % 5;
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
                    PowerupType t = rand() % 5;
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
            if (powerups[i].picked_up && !powerups[i].sound_played) {
                switch(powerups[i].type){
                    case POWERUP_EXTRA_LIFE:
                        play_sound(SOUND_EXTRALIFE);
                        break;
                    case POWERUP_SPEED_BOOST:
                        play_sound(SOUND_SPEED);
                        break;
                    case POWERUP_FREEZE_ENEMIES:
                        play_sound(SOUND_FREEZE);
                        break;
                    case POWERUP_DOUBLE_DAMAGE:
                        play_sound(SOUND_DAMAGE);
                        break;
                    default:
                        break;
                }
                powerups[i].sound_played = true; // 👈 flagga att ljudet har spelats
            }            
        }
        update_effects(&effects, &player.speed, &player.damage, now, powerups);
        
        // Renderingsfasen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Rita bakgrund med grön tile
    int TILE_SIZE = 32;
    SDL_Rect src = {0, 0, TILE_SIZE, TILE_SIZE};  // Grön tile från spritesheet
    SDL_Rect dest;
    for (int y = 0; y < SCREEN_HEIGHT; y += TILE_SIZE) {
    for (int x = 0; x < SCREEN_WIDTH; x += TILE_SIZE) {
        dest.x = x;
        dest.y = y;
        dest.w = TILE_SIZE;
        dest.h = TILE_SIZE;
        SDL_RenderCopy(renderer, tex_tiles, &src, &dest);
    }
}
        
        draw_player(renderer, &player);
        draw_powerup_bars(renderer, &player, powerups, now);
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                draw_mob(renderer, &mobs[i], player.rect);
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
    SDL_DestroyTexture(tex_player);
    SDL_DestroyTexture(tex_mob);
    SDL_DestroyTexture(tex_tiles);
    cleanup_sound();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}