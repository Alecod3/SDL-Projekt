#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

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

int showMenu(SDL_Renderer *renderer, SDL_Window *window) {
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char* labels[3] = {"Single Player", "Multiplayer", "Settings"};

    // Initiera font
    TTF_Font* font = TTF_OpenFont("shlop.ttf", 28);
    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        return -1;
    }

    TTF_Font* titleFont = TTF_OpenFont("simbiot.ttf", 64);
if (!titleFont) {
    SDL_Log("Failed to load title font: %s", TTF_GetError());
}

    // Positionera knappar
    for (int i = 0; i < 3; i++) {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }

    while (inMenu) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                TTF_CloseFont(font);
                return -1;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        selected = (selected + 2) % 3;
                        break;
                    case SDLK_DOWN:
                        selected = (selected + 1) % 3;
                        break;
                    case SDLK_RETURN:
                        TTF_CloseFont(font);
                        return selected;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x;
                int my = event.motion.y;
                for (int i = 0; i < 3; i++) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) {
                        selected = i;
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int mx = event.button.x;
                    int my = event.button.y;
                    for (int i = 0; i < 3; i++) {
                        if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                            my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) {
                            TTF_CloseFont(font);
                            return i;
                        }
                    }
                }
            }
        }

        // Rendera meny
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

         // 🔺 Rendera rubriken högst upp
         if (titleFont) {
            SDL_Color red = {200, 0, 0};
            SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, "Zombie Game", red);
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);

            SDL_Rect titleRect = {
                SCREEN_WIDTH / 2 - titleSurface->w / 2,
                60, // Höjd från toppen
                titleSurface->w,
                titleSurface->h
            };

            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }

        for (int i = 0; i < 3; i++) {
            if (i == selected)
                SDL_SetRenderDrawColor(renderer, 180, 0, 0, 255);
            else
                SDL_SetRenderDrawColor(renderer, 90, 0, 0, 255);

            SDL_RenderFillRect(renderer, &buttons[i]);

            // Rendera text
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

int showGameOver(SDL_Renderer *renderer) {
    SDL_Event event;
    bool inGameOver = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char* labels[3] = {"Play Again", "Main Menu", "Exit Game"};

    TTF_Font* optionFont = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font* titleFont = TTF_OpenFont("simbiot.ttf", 64);

    if (!optionFont || !titleFont) {
        SDL_Log("Failed to load Game Over fonts: %s", TTF_GetError());
        if (optionFont) TTF_CloseFont(optionFont);
        if (titleFont)  TTF_CloseFont(titleFont);
    
        return 2;
    }

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
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) {
                        selected = i;
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) {
                        return i;
                    }
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
            if (i == selected) SDL_SetRenderDrawColor(renderer, 180, 0, 0, 255);
            else SDL_SetRenderDrawColor(renderer, 90, 0, 0, 255);
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

    TTF_CloseFont(titleFont);
    TTF_CloseFont(optionFont);
    return 0;
}

bool skipMenu = false;

int SDL_main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        SDL_Log("SDL_ttf init error: %s", TTF_GetError());
        SDL_Quit();
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

    if (!skipMenu) {
        int menuResult = showMenu(renderer, window);
        if (menuResult == -1 || menuResult == 1 || menuResult == 2) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return 0;
        }
    }
    
    srand(time(NULL));
    SDL_Rect player = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PLAYER_SIZE, PLAYER_SIZE};
    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS];
    int score = 0;
    int lives = MAX_HEALTH;

    for (int i = 0; i < MAX_MOBS; i++) {
        do {
            mobs[i].rect.x = rand() % (SCREEN_WIDTH - MOB_SIZE);
            mobs[i].rect.y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
            mobs[i].rect.w = MOB_SIZE;
            mobs[i].rect.h = MOB_SIZE;
            mobs[i].active = true;
        } while (check_mob_collision(&mobs[i].rect, mobs, i));
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
                if (lives <= 0) {
                running = false;

                int result = showGameOver(renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    if (result == 0) {
        skipMenu = true;           // ⬅ Skippa menyn
        return SDL_main(0, NULL);
    } else if (result == 1) {
        skipMenu = false;          // ⬅ Visa menyn igen
        return SDL_main(0, NULL);
    } else {
        return 0;
    }
}
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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
