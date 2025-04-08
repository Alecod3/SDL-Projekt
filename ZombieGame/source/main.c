#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
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

typedef struct {
    SDL_Rect rect;
    bool active;
    float dx, dy;
} Bullet;

typedef struct {
    SDL_Rect rect;
    bool active;
    int type;     // Typ 0–3, styr färg och liv
    int health;   // 1 eller 2 beroende på typ
} Mob;

bool check_mob_collision(SDL_Rect *mob, Mob *mobs, int mob_index) {
    for (int i = 0; i < MAX_MOBS; i++) {
        if (i != mob_index && mobs[i].active && SDL_HasIntersection(mob, &mobs[i].rect)) {
            return true;
        }
    }
    return false;
}

void shoot_bullet(Bullet bullets[], SDL_Rect player, int mouse_x, int mouse_y) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].rect.x = player.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
            bullets[i].rect.y = player.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
            bullets[i].rect.w = BULLET_SIZE;
            bullets[i].rect.h = BULLET_SIZE;
            bullets[i].active = true;

            float dx = mouse_x - (player.x + PLAYER_SIZE / 2);
            float dy = mouse_y - (player.y + PLAYER_SIZE / 2);
            float length = SDL_sqrtf(dx * dx + dy * dy);
            bullets[i].dx = BULLET_SPEED * (dx / length);
            bullets[i].dy = BULLET_SPEED * (dy / length);
            break;
        }
    }
}

int SDL_main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
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
            mobs[i].type = rand() % 4;
            mobs[i].health = (mobs[i].type == 3) ? 2 : 1;  // Lila fiender = 2 liv
        } while (check_mob_collision(&mobs[i].rect, mobs, i));
    }

    bool running = true;
    SDL_Event event;
    static bool spacePressed = false;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                shoot_bullet(bullets, player, mx, my);
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if ((state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W]) && player.y > 0) player.y -= PLAYER_SPEED;
        if ((state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S]) && player.y + PLAYER_SIZE < SCREEN_HEIGHT) player.y += PLAYER_SPEED;
        if ((state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) && player.x > 0) player.x -= PLAYER_SPEED;
        if ((state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) && player.x + PLAYER_SIZE < SCREEN_WIDTH) player.x += PLAYER_SPEED;

        if (state[SDL_SCANCODE_SPACE]) {
            if (!spacePressed) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                shoot_bullet(bullets, player, mx, my);
                spacePressed = true;
            }
        } else {
            spacePressed = false;
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
                bullets[i].rect.x += (int)bullets[i].dx;
                bullets[i].rect.y += (int)bullets[i].dy;

                if (bullets[i].rect.x < 0 || bullets[i].rect.x > SCREEN_WIDTH ||
                    bullets[i].rect.y < 0 || bullets[i].rect.y > SCREEN_HEIGHT) {
                    bullets[i].active = false;
                    continue;
                }

                for (int j = 0; j < MAX_MOBS; j++) {
                    if (mobs[j].active && SDL_HasIntersection(&bullets[i].rect, &mobs[j].rect)) {
                        mobs[j].health--;
                        if (mobs[j].health <= 0) {
                            mobs[j].active = false;
                            score += 100;
                        }
                        bullets[i].active = false;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                SDL_Rect new_pos = mobs[i].rect;
                if (player.x < mobs[i].rect.x) new_pos.x -= MOB_SPEED;
                if (player.x > mobs[i].rect.x) new_pos.x += MOB_SPEED;
                if (player.y < mobs[i].rect.y) new_pos.y -= MOB_SPEED;
                if (player.y > mobs[i].rect.y) new_pos.y += MOB_SPEED;

                if (!check_mob_collision(&new_pos, mobs, i) &&
                    new_pos.x >= 0 && new_pos.x + MOB_SIZE <= SCREEN_WIDTH &&
                    new_pos.y >= 0 && new_pos.y + MOB_SIZE <= SCREEN_HEIGHT) {
                    mobs[i].rect = new_pos;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &player);

        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                switch (mobs[i].type) {
                    case 0: SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); break;     // Röd
                    case 1: SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); break;     // Blå
                    case 2: SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); break;   // Orange
                    case 3: SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255); break;   // Lila (2 liv)
                }
                SDL_RenderFillRect(renderer, &mobs[i].rect);
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                SDL_RenderFillRect(renderer, &bullets[i].rect);
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect healthBarBackground = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBarBackground);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect healthBar = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, (HEALTH_BAR_WIDTH * lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBar);

        printf("Score: %d | Lives: %d\n", score, lives);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
