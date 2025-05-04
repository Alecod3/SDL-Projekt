#include "network.h"
#include <SDL2/SDL_ttf.h>
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

#define NET_PORT 12345
#define PACKET_SIZE 32

static void render_text(SDL_Renderer *renderer,
                        TTF_Font *font,
                        const char *msg,
                        int x,
                        int y)
{
    SDL_Color color = {255, 255, 255, 255}; // vit text
    SDL_Surface *surf = TTF_RenderText_Blended(font, msg, color);
    if (!surf)
    {
        SDL_Log("TTF_RenderText_Blended failed: %s", TTF_GetError());
        return;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (!tex)
    {
        SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
        return;
    }
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

NetMode show_multiplayer_menu(SDL_Renderer *renderer, TTF_Font *font);
char *prompt_for_ip(SDL_Renderer *renderer, TTF_Font *font);
void wait_for_client(SDL_Renderer *renderer, TTF_Font *font);

bool skipMenu = false;
SDL_Texture *tex_player = NULL;
SDL_Texture *tex_mob = NULL;
SDL_Texture *tex_tiles = NULL;

int showSettings(SDL_Renderer *renderer, SDL_Window *window);

// Bullet-ADT (här hålls den enkelt i main, men den kan även brytas ut)
typedef struct
{
    SDL_Rect rect;
    bool active;
    float dx, dy;
} Bullet;

int showMenu(SDL_Renderer *renderer, SDL_Window *window)
{
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char *labels[3] = {"Single Player", "Multiplayer", "Settings"};

    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);

    for (int i = 0; i < 3; i++)
    {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }

    while (inMenu)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return -1;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP)
                    selected = (selected + 2) % 3;
                if (event.key.keysym.sym == SDLK_DOWN)
                    selected = (selected + 1) % 3;
                if (event.key.keysym.sym == SDLK_RETURN)
                {
                    if (selected == 2)
                    { // Settings
                        showSettings(renderer, window);
                    }
                    else
                    {
                        return selected;
                    }
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        return i;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont)
        {
            SDL_Color red = {200, 0, 0};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Zombie Game", red);
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }

        for (int i = 0; i < 3; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color color = {255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(font, labels[i], color);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h};
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

int showSettings(SDL_Renderer *renderer, SDL_Window *window)
{
    SDL_Event event;
    bool inSettings = true;
    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 48);

    if (!font || !titleFont)
    {
        SDL_Log("Misslyckades med att ladda font i inställningsmeny: %s", TTF_GetError());
        return 0;
    }

    while (inSettings)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return -1;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN)
                {
                    inSettings = false; // Gå tillbaka till menyn
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // --- Rubrik: Settings ---
        SDL_Color white = {255, 255, 255};
        SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Settings", white);
        SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 50, titleSurface->w, titleSurface->h};
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_FreeSurface(titleSurface);
        SDL_DestroyTexture(titleTexture);

        // --- Volym-rubrik till vänster ---
        SDL_Surface *volumeSurface = TTF_RenderText_Blended(font, "Volume", white);
        SDL_Texture *volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
        SDL_Rect volumeRect = {100, 180, volumeSurface->w, volumeSurface->h};
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

int showGameOver(SDL_Renderer *renderer)
{
    SDL_Event event;
    bool inGameOver = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char *labels[3] = {"Play Again", "Main Menu", "Exit Game"};

    TTF_Font *optionFont = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);

    for (int i = 0; i < 3; i++)
    {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }

    while (inGameOver)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return 2;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP)
                    selected = (selected + 2) % 3;
                if (event.key.keysym.sym == SDLK_DOWN)
                    selected = (selected + 1) % 3;
                if (event.key.keysym.sym == SDLK_RETURN)
                    return selected;
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        return i;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont)
        {
            SDL_Color red = {200, 0, 0};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "GAME OVER", red);
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }

        for (int i = 0; i < 3; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color color = {255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(optionFont, labels[i], color);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h};
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

int main(int argc, char *argv[])
{
    // Initiera SDL2 och SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("SDL kunde inte initieras: %s", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        SDL_Log("SDL_image kunde inte initieras: %s", IMG_GetError());
        return 1;
    }

    if (TTF_Init() == -1)
    {
        SDL_Log("SDL_ttf init error: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Zombie Game",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        SDL_Log("Fönster kunde inte skapas: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        SDL_Log("Renderer kunde inte skapas: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    TTF_Font *uiFont = TTF_OpenFont("shlop.ttf", 28);
    if (!uiFont)
    {
        SDL_Log("Misslyckades ladda font för meny: %s", TTF_GetError());
        // Städa upp innan exit
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    NetMode mode = show_multiplayer_menu(renderer, uiFont);

    char *server_ip = NULL;
    if (mode == MODE_JOIN)
    {
        server_ip = prompt_for_ip(renderer, uiFont);
    }

    if (network_init(mode, server_ip) < 0)
    {
        SDL_Log("Network init failed");
        return 1;
    }

    if (mode == MODE_HOST)
    {
        wait_for_client(renderer, uiFont);
    }

    skipMenu = true;

    Player playerLocal = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                       PLAYER_SIZE, DEFAULT_PLAYER_SPEED,
                                       DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    Player playerRemote = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                        PLAYER_SIZE, DEFAULT_PLAYER_SPEED,
                                        DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    playerRemote.tint = (SDL_Color){200, 80, 80, 255};

    if (mode == MODE_JOIN)
    {
        // Skicka ett första paket för att hostens wait_for_client() ska snappa upp dig
        network_send(playerLocal.rect.x, playerLocal.rect.y, playerLocal.aim_angle);
    }

    init_sound();

    play_music("source/spelmusik.wav");

    if (!skipMenu)
    {
        int menuResult = showMenu(renderer, window);
        if (menuResult != 0)
        {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 0;
        }
    }

    // Ladda in powerup-texturer (variablerna tex_extralife, etc. definieras i powerups.c som externa)
    tex_extralife = IMG_LoadTexture(renderer, "resources/extralife.png");
    tex_extraspeed = IMG_LoadTexture(renderer, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(renderer, "resources/doubledamage.png");
    tex_freezeenemies = IMG_LoadTexture(renderer, "resources/freezeenemies.png");

    tex_tiles = IMG_LoadTexture(renderer, "resources/tiles.png");
    if (!tex_tiles)
    {
        SDL_Log("Kunde inte ladda in tilesheet: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    tex_player = IMG_LoadTexture(renderer, "resources/hitman1.png");
    if (!tex_player)
    {
        SDL_Log("Kunde inte ladda in spelartextur: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    tex_mob = IMG_LoadTexture(renderer, "resources/zombie1.png");
    if (!tex_mob)
    {
        SDL_Log("Kunde inte ladda in fiendetextur: %s", SDL_GetError());
        SDL_DestroyTexture(tex_player); // du har laddat spelartestur innan
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (!tex_extralife || !tex_extraspeed || !tex_doubledamage)
    {
        SDL_Log("Kunde inte ladda in powerup-texturer: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    srand((unsigned int)time(NULL));

    // Skapa spelaren via Player-ADT; DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE och MAX_HEALTH ska vara definierade (här antas de komma från powerups.h)
    // Player player = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PLAYER_SIZE, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);

    // Skapa bullets, mobs och powerups
    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS];
    for (int i = 0; i < MAX_MOBS; i++)
    {
        int x, y, type, health;
        do
        {
            x = rand() % (SCREEN_WIDTH - MOB_SIZE);
            y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
            type = rand() % 4; // Antag 4 typer
            health = (type == 3) ? 2 : 1;
            mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
        } while (check_mob_collision(&mobs[i].rect, mobs, i));
    }

    Powerup powerups[MAX_POWERUPS];
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
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
    ActiveEffects effects = {false, 0, false, 0};

    Uint32 now, last_mob_spawn_time = 0, last_powerup_spawn_time = 0;

    if (mode == MODE_HOST)
    {
        wait_for_client(renderer, uiFont);

        // Skicka befintliga mobs
        for (int i = 0; i < MAX_MOBS; i++)
            if (mobs[i].active)
                network_send_spawn_mob(i,
                                       mobs[i].rect.x, mobs[i].rect.y,
                                       mobs[i].type, mobs[i].health);

        // Skicka befintliga powerups
        for (int i = 0; i < MAX_POWERUPS; i++)
            if (powerups[i].active)
                network_send_spawn_powerup(i,
                                           powerups[i].rect.x, powerups[i].rect.y,
                                           powerups[i].type);
    }

    // Spel-loop
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                // Skjut en bullet med spelarens damage
                // (Skjutfunktionen finns i main, vi använder lokala bullets array)
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active)
                    {
                        bullets[i].rect.x = playerLocal.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.y = playerLocal.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;

                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;

                        // 1) Räkna ut rörelseriktning först
                        {
                            float dx = (float)mx - (playerLocal.rect.x + PLAYER_SIZE / 2);
                            float dy = (float)my - (playerLocal.rect.y + PLAYER_SIZE / 2);
                            float length = sqrtf(dx * dx + dy * dy);
                            bullets[i].dx = BULLET_SPEED * (dx / length);
                            bullets[i].dy = BULLET_SPEED * (dy / length);
                        }

                        // 2) Skicka kulan med rätt dx/dy
                        network_send_fire_bullet(
                            i,
                            bullets[i].rect.x, bullets[i].rect.y,
                            bullets[i].dx, bullets[i].dy);

                        play_sound(SOUND_SHOOT);
                        break;
                    }
                }
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);

        // ——— Rå nätverks‐dispatch istället för network_receive() ———
        while (SDLNet_UDP_Recv(netSocket, pktIn))
        {
            Uint8 msg = pktIn->data[0];
            int off = 1;
            if (msg == MSG_POS)
            {
                int rx, ry;
                float rang;
                memcpy(&rx, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&ry, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&rang, pktIn->data + off, sizeof(float));
                off += sizeof(float);
                playerRemote.rect.x = rx;
                playerRemote.rect.y = ry;
                playerRemote.aim_angle = rang;
            }
            else if (msg == MSG_SPAWN_MOB)
            {
                int idx, x, y, type, health;
                memcpy(&idx, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&x, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&y, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&type, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&health, pktIn->data + off, sizeof(int));
                if (idx >= 0 && idx < MAX_MOBS)
                    mobs[idx] = create_mob(x, y, MOB_SIZE, type, health);
            }
            else if (msg == MSG_SPAWN_PWR)
            {
                int idx, x, y, ptype;
                memcpy(&idx, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&x, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&y, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&ptype, pktIn->data + off, sizeof(int));
                if (idx >= 0 && idx < MAX_POWERUPS)
                    powerups[idx] = create_powerup(ptype, x, y);
            }
            else if (msg == MSG_FIRE_BULLET)
            {
                int idx, x, y;
                float dx, dy;

                memcpy(&idx, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&x, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&y, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&dx, pktIn->data + off, sizeof(float));
                off += sizeof(float);
                memcpy(&dy, pktIn->data + off, sizeof(float));
                off += sizeof(float);

                bullets[idx].rect.x = x;
                bullets[idx].rect.y = y;
                bullets[idx].dx = dx;
                bullets[idx].dy = dy;
                bullets[idx].rect.w = BULLET_SIZE;
                bullets[idx].rect.h = BULLET_SIZE;
                bullets[idx].active = true;
            }

            else if (msg == MSG_REMOVE_PWR)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < MAX_POWERUPS)
                    powerups[idx].active = false;
            }

            else if (msg == MSG_MOB_POS)
            {
                int idx, mx, my;
                memcpy(&idx, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&mx, pktIn->data + off, sizeof(int));
                off += sizeof(int);
                memcpy(&my, pktIn->data + off, sizeof(int)); // off += sizeof(int);

                if (idx >= 0 && idx < MAX_MOBS)
                {
                    mobs[idx].rect.x = mx;
                    mobs[idx].rect.y = my;
                }
            }
            else if (msg == MSG_REMOVE_MOB)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < MAX_MOBS)
                    mobs[idx].active = false;
            }
            else if (msg == MSG_REMOVE_BULLET)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < MAX_BULLETS)
                    bullets[idx].active = false;
            }
        }
        update_player(&playerLocal, state);

        int mx, my;
        SDL_GetMouseState(&mx, &my);
        float dx = mx - (playerLocal.rect.x + PLAYER_SIZE / 2);
        float dy = my - (playerLocal.rect.y + PLAYER_SIZE / 2);
        playerLocal.aim_angle = atan2f(dy, dx) * 180.0f / (float)M_PI;
        network_send(playerLocal.rect.x, playerLocal.rect.y, playerLocal.aim_angle);
        network_send(playerLocal.rect.x, playerLocal.rect.y, playerLocal.aim_angle);

        if (state[SDL_SCANCODE_SPACE])
        {
            if (!spacePressed)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active)
                    {
                        bullets[i].rect.x = playerLocal.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.y = playerLocal.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;

                        // 1) Beräkna dx/dy
                        {
                            float dx = (float)mx - (playerLocal.rect.x + PLAYER_SIZE / 2);
                            float dy = (float)my - (playerLocal.rect.y + PLAYER_SIZE / 2);
                            float length = sqrtf(dx * dx + dy * dy);
                            bullets[i].dx = BULLET_SPEED * (dx / length);
                            bullets[i].dy = BULLET_SPEED * (dy / length);
                        }

                        // 2) Skicka med korrekta värden
                        network_send_fire_bullet(
                            i,
                            bullets[i].rect.x, bullets[i].rect.y,
                            bullets[i].dx, bullets[i].dy);

                        play_sound(SOUND_SHOOT);
                        break;
                    }
                }
                spacePressed = true;
            }
        }
        else
        {
            spacePressed = false;
        }

        // Kollision och attack‑hantering mellan spelare och mobs
        for (int i = 0; i < MAX_MOBS; i++)
        {
            if (!mobs[i].active)
                continue;

            if (mobs[i].attacking)
            {
                Uint32 now = SDL_GetTicks();
                // Slå bara om attack_interval passerat
                if (now - mobs[i].last_attack_time >= mobs[i].attack_interval)
                {
                    if (mobs[i].type == 3)
                    {
                        playerLocal.lives -= 2;
                    }
                    else
                    {
                        playerLocal.lives--;
                    }
                    mobs[i].last_attack_time = now;

                    // Kolla om spelaren dör
                    if (playerLocal.lives <= 0)
                    {
                        int result = showGameOver(renderer);
                        if (result == 0)
                        {
                            skipMenu = true;
                            return main(argc, argv);
                        }
                        else if (result == 1)
                        {
                            skipMenu = false;
                            return main(argc, argv);
                        }
                        else
                        {
                            running = false;
                        }
                    }
                }
            }
        }

        // Uppdatera bullets: förflytta och kolla kollision med mobs
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                bullets[i].rect.x += (int)bullets[i].dx;
                bullets[i].rect.y += (int)bullets[i].dy;
                if (bullets[i].rect.x < 0 || bullets[i].rect.x > SCREEN_WIDTH ||
                    bullets[i].rect.y < 0 || bullets[i].rect.y > SCREEN_HEIGHT)
                {
                    bullets[i].active = false;

                    network_send_remove_bullet(i);
                    continue;
                }
                for (int j = 0; j < MAX_MOBS; j++)
                {
                    if (mobs[j].active && SDL_HasIntersection(&bullets[i].rect, &mobs[j].rect))
                    {
                        mobs[j].health -= playerLocal.damage;
                        if (mobs[j].health <= 0)
                        {
                            mobs[j].active = false;
                            network_send_remove_mob(j);
                            network_send_remove_bullet(i);
                            score += 100;
                            if (rand() % 4 == 0)
                            { // 25% chans att en mob släpper en powerup vid död
                                for (int k = 0; k < MAX_POWERUPS; k++)
                                {
                                    if (!powerups[k].active && !powerups[k].picked_up)
                                    {
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

        for (int i = 0; i < MAX_MOBS; i++)
        {
            if (!mobs[i].active || freeze_active)
                continue;

            // beräkna kvadrerad distans
            float dxL = (playerLocal.rect.x - mobs[i].rect.x);
            float dyL = (playerLocal.rect.y - mobs[i].rect.y);
            float distL = dxL * dxL + dyL * dyL;

            float dxR = (playerRemote.rect.x - mobs[i].rect.x);
            float dyR = (playerRemote.rect.y - mobs[i].rect.y);
            float distR = dxR * dxR + dyR * dyR;

            SDL_Rect target = (distR < distL)
                                  ? playerRemote.rect
                                  : playerLocal.rect;

            update_mob(&mobs[i], target);
        }

        // Spawna nya mobs var 1,5 sekund
        now = SDL_GetTicks();
        if (mode == MODE_HOST && now - last_mob_spawn_time >= 1500)
        {
            for (int i = 0; i < MAX_MOBS; i++)
            {
                if (!mobs[i].active)
                {
                    int x = rand() % (SCREEN_WIDTH - MOB_SIZE);
                    int y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
                    int type = rand() % 4;
                    int health = (type == 3) ? 2 : 1;

                    // 1) Skapa lokalt
                    mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
                    // 2) Synka till klienten med exakta slot-index
                    network_send_spawn_mob(i, x, y, type, health);

                    break;
                }
            }
            last_mob_spawn_time = now;
        }

        // Spawna nya powerups var 5 sekund
        if (mode == MODE_HOST && now - last_powerup_spawn_time >= 5000)
        {
            for (int i = 0; i < MAX_POWERUPS; i++)
            {
                if (!powerups[i].active)
                {
                    int t = rand() % 3;
                    int x = rand() % (SCREEN_WIDTH - 32);
                    int y = rand() % (SCREEN_HEIGHT - 32);

                    // 1) Skapa lokalt
                    powerups[i] = create_powerup(t, x, y);

                    // 2) Synka med klienten – skicka index i först
                    network_send_spawn_powerup(i, x, y, t);

                    break;
                }
            }
            last_powerup_spawn_time = now;
        }
        // Hantera kollisioner och effekter för powerups
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            check_powerup_collision(&powerups[i], playerLocal.rect, &playerLocal.lives, &playerLocal.speed, &playerLocal.damage, now, &effects);
            if (powerups[i].picked_up && !powerups[i].sound_played)
            {
                // 1) Ta bort powerup‑slot lokalt och synka med peer
                powerups[i].active = false;
                network_send_remove_powerup(i);

                // 2) Spela upp rätt ljud
                switch (powerups[i].type)
                {
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

                // 3) Flagga så vi inte spelar ljudet igen
                powerups[i].sound_played = true;
            }
        }
        update_effects(&effects, &playerLocal.speed, &playerLocal.damage, now, powerups);

        // Renderingsfasen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Rita bakgrund med grön tile
        int TILE_SIZE = 32;
        SDL_Rect src = {0, 0, TILE_SIZE, TILE_SIZE}; // Grön tile från spritesheet
        SDL_Rect dest;
        for (int y = 0; y < SCREEN_HEIGHT; y += TILE_SIZE)
        {
            for (int x = 0; x < SCREEN_WIDTH; x += TILE_SIZE)
            {
                dest.x = x;
                dest.y = y;
                dest.w = TILE_SIZE;
                dest.h = TILE_SIZE;
                SDL_RenderCopy(renderer, tex_tiles, &src, &dest);
            }
        }

        // draw_player(renderer, &playerLocal);
        // draw_player(renderer, &playerRemote);

        SDL_Point center = {PLAYER_SIZE / 2, PLAYER_SIZE / 2};

        SDL_RenderCopyEx(renderer, tex_player, NULL, &playerLocal.rect,
                         playerLocal.aim_angle, &center, SDL_FLIP_NONE);

        SDL_RenderCopyEx(renderer, tex_player, NULL, &playerRemote.rect,
                         playerRemote.aim_angle, &center, SDL_FLIP_NONE);

        draw_powerup_bars(renderer, &playerLocal, powerups, now);

        for (int i = 0; i < MAX_MOBS; i++)
        {
            if (mobs[i].active)
            {
                draw_mob(renderer, &mobs[i], playerLocal.rect);
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &bullets[i].rect);
            }
        }

        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            draw_powerup(renderer, &powerups[i]);
        }

        // Rita en enkel livsbar
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect healthBarBackground = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBarBackground);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect healthBar = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, (HEALTH_BAR_WIDTH * playerLocal.lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBar);

        // Skriver ut score och lives på konsolen
        printf("Score: %d | Lives: %d\n", score, playerLocal.lives);
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
    TTF_CloseFont(uiFont);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

NetMode show_multiplayer_menu(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_Rect host_btn = {100, 150, 200, 60};
    SDL_Rect join_btn = {100, 250, 200, 60};
    SDL_Event e;
    while (1)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                exit(0);
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int x = e.button.x, y = e.button.y;
                if (SDL_PointInRect(&(SDL_Point){x, y}, &host_btn))
                    return MODE_HOST;
                if (SDL_PointInRect(&(SDL_Point){x, y}, &join_btn))
                    return MODE_JOIN;
            }
        }
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        // Rita knappar
        SDL_SetRenderDrawColor(renderer, 80, 80, 200, 255);
        SDL_RenderFillRect(renderer, &host_btn);
        SDL_RenderFillRect(renderer, &join_btn);

        // Rita text
        render_text(renderer, font, "Host Game", host_btn.x + 20, host_btn.y + 20);
        render_text(renderer, font, "Join Game", join_btn.x + 20, join_btn.y + 20);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

char *prompt_for_ip(SDL_Renderer *renderer, TTF_Font *font)
{
    // 1) Starta textinput och släng gamla events
    SDL_StartTextInput();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

    // 2) Alokera buffert
    char *ip = calloc(256, 1);
    size_t len = 0;
    SDL_Event e;

    // 3) Rektangel för inputfältet
    SDL_Rect inputRect = {100, 150, 300, 32};

    while (1)
    {
        // 4) Hämta och hantera alla events
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                exit(0);

            if (e.type == SDL_TEXTINPUT)
            {
                // Lägg på nya tecken
                if (len + strlen(e.text.text) < 255)
                {
                    strcat(ip, e.text.text);
                    len = strlen(ip);
                }
            }
            else if (e.type == SDL_KEYDOWN)
            {
                // Backspace
                if (e.key.keysym.sym == SDLK_BACKSPACE && len > 0)
                {
                    ip[--len] = '\0';
                }
                // Vanlig Enter eller Enter på knappsatsen
                else if (e.key.keysym.sym == SDLK_RETURN ||
                         e.key.keysym.sym == SDLK_KP_ENTER)
                {
                    SDL_StopTextInput();
                    return ip;
                }
            }
        }

        // 5) Rendera prompt + inputfältet
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        // Skriv prompt‐text
        render_text(renderer, font, "Enter server IP:", 100, 110);

        // Rita fältets ram
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &inputRect);

        // Skriv IP‐strängen inuti
        render_text(renderer, font, ip, inputRect.x + 5, inputRect.y + 5);

        SDL_RenderPresent(renderer);

        // Liten paus så vi inte skriker CPU
        SDL_Delay(16);
    }
}

void wait_for_client(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_Event e;
    int x, y;
    while (1)
    {
        // hantera quit
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                exit(0);
        }
        // om paket mottaget -> bryt
        if (network_receive(&x, &y))
            break;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render_text(renderer, font, "Waiting for client...", 100, 100);
        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }
}