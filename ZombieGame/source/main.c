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
#define MAX_MOBS 40
#define MOB_SPAWN_INTERVAL_MS 1500
#define MOBS_SPAWN_PER_WAVE 3
#define MAX_POWERUPS 5
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

    Player *playerLocal = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                        PLAYER_SIZE, DEFAULT_PLAYER_SPEED,
                                        DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    Player *playerRemote = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                         PLAYER_SIZE, DEFAULT_PLAYER_SPEED,
                                         DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    player_set_tint(playerRemote, (SDL_Color){200, 80, 80, 255}); // ✅ korrekt ADT-stil

    if (mode == MODE_JOIN)
    {
        // Skicka ett första paket för att hostens wait_for_client() ska snappa upp dig
        network_send(player_get_x(playerLocal), player_get_y(playerLocal), player_get_aim_angle(playerLocal));
    }

    SoundSystem *audio = SoundSystem_create();
    if (!audio)
    {
        fprintf(stderr, "Kunde inte initiera ljud\n");
        return 1;
    }

    SoundSystem_playMusic(audio, "source/spelmusik.wav");

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
        SDL_Log("Kunde inte ladda in powerup-texturer: %s", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    srand((unsigned int)time(NULL));

    Uint32 start_time = SDL_GetTicks();
    int current_max_mobs = 10; // Börja med 10 mobs
    int last_sent_max_mobs = 0;

    // Skapa bullets, mobs och powerups
    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS];
    for (int i = 0; i < current_max_mobs; i++)
    {
        int x, y, type, health;
        // välj slumpvis sida: 0=vänster,1=höger,2=ovan,3=under
        int side = rand() % 4;
        switch (side)
        {
        case 0:
            x = -MOB_SIZE;
            y = rand() % SCREEN_HEIGHT;
            break;
        case 1:
            x = SCREEN_WIDTH;
            y = rand() % SCREEN_HEIGHT;
            break;
        case 2:
            x = rand() % SCREEN_WIDTH;
            y = -MOB_SIZE;
            break;
        case 3:
            x = rand() % SCREEN_WIDTH;
            y = SCREEN_HEIGHT;
            break;
        }
        type = rand() % 4;
        health = (type == 3) ? 2 : 1;
        // hoppa över kollisionskontroll vid init
        mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
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
        for (int i = 0; i < current_max_mobs; i++)
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
                if (player_is_reloading(playerLocal))
                    break;

                if (player_get_ammo(playerLocal) <= 0)
                {
                    player_start_reload(playerLocal, SDL_GetTicks());
                    break;
                }

                player_set_ammo(playerLocal, player_get_ammo(playerLocal) - 1);

                int mx, my;
                SDL_GetMouseState(&mx, &my);

                for (int i = 0; i < MAX_BULLETS; i++)
                {

                    if (!bullets[i].active)
                    {

                        int center_x = player_get_center_x(playerLocal);
                        int center_y = player_get_center_y(playerLocal);

                        bullets[i].rect.x = center_x - BULLET_SIZE / 2;
                        bullets[i].rect.y = center_y - BULLET_SIZE / 2;

                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;

                        // 1) Räkna ut rörelseriktning först
                        {
                            float dx = (float)mx - center_x;
                            float dy = (float)my - center_y;
                            float length = sqrtf(dx * dx + dy * dy);
                            bullets[i].dx = BULLET_SPEED * (dx / length);
                            bullets[i].dy = BULLET_SPEED * (dy / length);
                        }

                        // 2) Skicka kulan med rätt dx/dy
                        network_send_fire_bullet(
                            i,
                            bullets[i].rect.x, bullets[i].rect.y,
                            bullets[i].dx, bullets[i].dy);

                        SoundSystem_playEffect(audio, SOUND_SHOOT);
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
                player_set_position(playerRemote, rx, ry);
                player_set_aim_angle(playerRemote, rang);
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
                if (idx >= 0 && idx < current_max_mobs)
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
            else if (msg == MSG_REMOVE_MOB)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < current_max_mobs)
                    mobs[idx].active = false;
            }
            else if (msg == MSG_FREEZE)
            {
                effects.freeze_active = true;
                effects.freeze_start_time = SDL_GetTicks();
            }
            else if (msg == MSG_DAMAGE)
            {
                int amount;
                memcpy(&amount, pktIn->data + 1, sizeof(int));
                // Båda spelarna tar damage
                player_set_lives(playerLocal, player_get_lives(playerLocal) - amount);
                player_set_lives(playerRemote, player_get_lives(playerRemote) - amount);

                // Synka det nya HP-värdet
                network_send_set_hp(player_get_lives(playerLocal));

                // Kontrollera om spelet är över
                if (player_get_lives(playerLocal) <= 0)
                {
                    network_send_game_over();
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
            else if (msg == MSG_SET_HP)
            {
                int hp;
                memcpy(&hp, pktIn->data + 1, sizeof(int));
                // Sätt båda spelarnas HP till samma värde
                player_set_lives(playerLocal, hp);
                player_set_lives(playerRemote, hp);
                // Kontrollera om spelet är över
                if (player_get_lives(playerLocal) <= 0)
                {
                    network_send_game_over();
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
            else if (msg == MSG_GAME_OVER)
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
            else if (msg == MSG_SET_MAX_MOBS)
            {
                int new_max;
                memcpy(&new_max, pktIn->data + 1, sizeof(int));
                current_max_mobs = new_max;
            }
            else if (msg == MSG_REMOVE_BULLET)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < MAX_BULLETS)
                    bullets[idx].active = false;
            }
        }
        update_player(playerLocal, state);

        if (player_is_reloading(playerLocal))
        {
            if (SDL_GetTicks() - player_get_reload_start_time(playerLocal) >= 2000)
            {
                player_finish_reload(playerLocal);
            }
        }

        int mx, my;
        SDL_GetMouseState(&mx, &my);
        int cx = player_get_center_x(playerLocal);
        int cy = player_get_center_y(playerLocal);
        float dx = mx - cx;
        float dy = my - cy;
        float angle = atan2f(dy, dx) * 180.0f / (float)M_PI;
        player_set_aim_angle(playerLocal, angle);
        network_send(player_get_x(playerLocal), player_get_y(playerLocal), angle);

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
                        int px = player_get_center_x(playerLocal);
                        int py = player_get_center_y(playerLocal);
                        bullets[i].rect.x = px - BULLET_SIZE / 2;
                        bullets[i].rect.y = py - BULLET_SIZE / 2;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;

                        // 1) Beräkna dx/dy
                        {
                            int center_x = player_get_center_x(playerLocal);
                            int center_y = player_get_center_y(playerLocal);
                            float dx = (float)mx - center_x;
                            float dy = (float)my - center_y;
                            float length = sqrtf(dx * dx + dy * dy);
                            bullets[i].dx = BULLET_SPEED * (dx / length);
                            bullets[i].dy = BULLET_SPEED * (dy / length);
                        }

                        // 2) Skicka med korrekta värden
                        network_send_fire_bullet(
                            i,
                            bullets[i].rect.x, bullets[i].rect.y,
                            bullets[i].dx, bullets[i].dy);

                        SoundSystem_playEffect(audio, SOUND_SHOOT);
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

        // Kollision och attack‑hantering mellan spelare och mobs (endast på host)
        if (mode == MODE_HOST)
        {
            for (int i = 0; i < current_max_mobs; i++)
            {
                if (!mobs[i].active)
                    continue;

                if (mobs[i].attacking)
                {
                    Uint32 now = SDL_GetTicks();
                    // Slå bara om attack_interval passerat
                    if (now - mobs[i].last_attack_time >= mobs[i].attack_interval)
                    {
                        int damage = (mobs[i].type == 3) ? 2 : 1;
                        player_set_lives(playerLocal, player_get_lives(playerLocal) - damage);
                        player_set_lives(playerRemote, player_get_lives(playerRemote) - damage);
                        network_send_damage(damage); // Skicka damage till klienten
                        mobs[i].last_attack_time = now;

                        // Kontrollera om spelet är över
                        if (player_get_lives(playerLocal) <= 0)
                        {
                            network_send_game_over();
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
                for (int j = 0; j < current_max_mobs; j++)
                {
                    if (mobs[j].active && SDL_HasIntersection(&bullets[i].rect, &mobs[j].rect))
                    {
                        mobs[j].health -= player_get_damage(playerLocal);
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
                                    if (!powerups[j].active && !powerups[j].picked_up)
                                    {
                                        PowerupType t = rand() % 4;
                                        powerups[j] = create_powerup(t, mobs[j].rect.x, mobs[j].rect.y);
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
        for (int i = 0; i < current_max_mobs; i++)
        {
            if (!mobs[i].active || freeze_active)
                continue;
            SDL_Rect rectL = player_get_rect(playerLocal);
            SDL_Rect rectR = player_get_rect(playerRemote);
            // Beräkna avstånd till Local
            float dxL = (rectL.x + rectL.w / 2.0f) - (mobs[i].rect.x + mobs[i].rect.w / 2.0f);
            float dyL = (rectL.y + rectL.h / 2.0f) - (mobs[i].rect.y + mobs[i].rect.h / 2.0f);
            float distL = sqrtf(dxL * dxL + dyL * dyL);

            // Beräkna avstånd till Remote
            float dxR = (rectR.x + rectR.w / 2.0f) - (mobs[i].rect.x + mobs[i].rect.w / 2.0f);
            float dyR = (rectR.y + rectR.h / 2.0f) - (mobs[i].rect.y + mobs[i].rect.h / 2.0f);
            float distR = sqrtf(dxR * dxR + dyR * dyR);

            // Välj den närmaste spelaren som mål
            SDL_Rect target = (distL < distR) ? player_get_rect(playerLocal) : player_get_rect(playerRemote);

            // Uppdatera mobben mot target
            update_mob(&mobs[i], target);
        }

        // Spawna nya mobs var 1,5 sekund
        now = SDL_GetTicks();

        // Uppdatera tillåtet antal mobs var 10:e sekund (max 40)
        Uint32 elapsed = (SDL_GetTicks() - start_time) / 10000;
        int new_max = 10 + elapsed * 10;
        if (new_max > 40)
            new_max = 40;

        if (mode == MODE_HOST && new_max != current_max_mobs)
        {
            current_max_mobs = new_max;
            network_send_set_max_mobs(current_max_mobs);
        }

        if (mode == MODE_HOST && now - last_mob_spawn_time >= MOB_SPAWN_INTERVAL_MS)
        {
            int spawned = 0;
            for (int i = 0; i < current_max_mobs && spawned < MOBS_SPAWN_PER_WAVE; i++)
            {
                if (!mobs[i].active)
                {
                    int x, y, type, health;
                    int side = rand() % 4;
                    switch (side)
                    {
                    case 0:
                        x = -MOB_SIZE;
                        y = rand() % SCREEN_HEIGHT;
                        break;
                    case 1:
                        x = SCREEN_WIDTH;
                        y = rand() % SCREEN_HEIGHT;
                        break;
                    case 2:
                        x = rand() % SCREEN_WIDTH;
                        y = -MOB_SIZE;
                        break;
                    case 3:
                        x = rand() % SCREEN_WIDTH;
                        y = SCREEN_HEIGHT;
                        break;
                    }
                    type = rand() % 4;
                    health = (type == 3) ? 2 : 1;

                    mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
                    network_send_spawn_mob(i, x, y, type, health);

                    spawned++;
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
            check_powerup_collision(&powerups[i], playerLocal, now, &effects);
            if (powerups[i].picked_up && !powerups[i].sound_played)
            {
                // 1) Ta bort powerup‑slot lokalt och synka med peer
                powerups[i].active = false;
                network_send_remove_powerup(i);
                if (powerups[i].type == POWERUP_FREEZE_ENEMIES)
                    network_send_freeze();

                // 2) Spela upp rätt ljud
                switch (powerups[i].type)
                {
                case POWERUP_EXTRA_LIFE:
                    SoundSystem_playEffect(audio, SOUND_EXTRALIFE);
                    break;
                case POWERUP_SPEED_BOOST:
                    SoundSystem_playEffect(audio, SOUND_SPEED);
                    break;
                case POWERUP_FREEZE_ENEMIES:
                    SoundSystem_playEffect(audio, SOUND_FREEZE);
                    break;
                case POWERUP_DOUBLE_DAMAGE:
                    SoundSystem_playEffect(audio, SOUND_DAMAGE);
                    break;
                default:
                    break;
                }

                // 3) Flagga så vi inte spelar ljudet igen
                powerups[i].sound_played = true;
            }
        }
        update_effects(&effects, playerLocal, now, powerups);

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

        SDL_Point center = {PLAYER_SIZE / 2, PLAYER_SIZE / 2};

        SDL_Rect rect = player_get_rect(playerLocal);
        SDL_RenderCopyEx(renderer, tex_player, NULL, &rect,
                         player_get_aim_angle(playerLocal), &center, SDL_FLIP_NONE);

        SDL_Rect rectRemote = player_get_rect(playerRemote);
        SDL_RenderCopyEx(renderer, tex_player, NULL, &rectRemote,
                         player_get_aim_angle(playerRemote), &center, SDL_FLIP_NONE);

        draw_powerup_bars(renderer, playerLocal, powerups, now);

        SDL_Rect player_rect = player_get_rect(playerLocal);
        for (int i = 0; i < current_max_mobs; i++)
        {
            if (mobs[i].active)
            {
                draw_mob(renderer, &mobs[i], player_rect);
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

        int barX = 10;
        int barY = 10;
        int barW = HEALTH_BAR_WIDTH;
        int barH = HEALTH_BAR_HEIGHT;

        // 1) Rita röd bakgrund (full bredd)
        SDL_Rect bg = {barX, barY, barW, barH};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &bg);

        // 2) Rita grön förgrund (proportionellt mot liv kvar)
        int lives = player_get_lives(playerLocal);
        float frac = (float)lives / (float)MAX_HEALTH;
        SDL_Rect fg = {barX, barY, (int)(barW * frac), barH};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &fg);

        // Skriver ut score och lives på konsolen
        printf("Score: %d | Lives: %d\n", score, lives);

        // Visar ammo //

        char ammo_text[32];
        int ammo = player_get_ammo(playerLocal);
        sprintf(ammo_text, "%d/30", ammo);

        SDL_Color color = {255, 255, 255, 255}; // vit
        TTF_Font *font_to_use = uiFont;

        // Beräkna position
        int text_width = 0, text_height = 0;
        TTF_SizeText(font_to_use, ammo_text, &text_width, &text_height);
        int x = SCREEN_WIDTH - text_width - 10;
        int y = SCREEN_HEIGHT - text_height - 10;

        // Rendera text
        SDL_Surface *surf = TTF_RenderText_Blended(font_to_use, ammo_text, color);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_FreeSurface(surf);
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
        //--------------------------------------------------------------------------//
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
    SoundSystem_destroy(audio);
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