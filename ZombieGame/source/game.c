#include "game.h"
#include "menu.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <windows.h>

extern bool skipMenu;
extern SDL_Texture *tex_player;
extern SDL_Texture *tex_mob;
extern SDL_Texture *tex_tiles;
extern SDL_Texture *tex_extralife;
extern SDL_Texture *tex_extraspeed;
extern SDL_Texture *tex_doubledamage;
extern SDL_Texture *tex_freezeenemies;

static void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *msg, int x, int y)
{
    SDL_Color color = {255, 255, 255, 255};
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

static bool is_position_valid(int x, int y, Player *playerLocal, Player *playerRemote)
{
    const int MIN_DISTANCE = 100;
    int px = player_get_center_x(playerLocal);
    int py = player_get_center_y(playerLocal);
    float dx = x - px;
    float dy = y - py;
    if (sqrtf(dx * dx + dy * dy) < MIN_DISTANCE)
    {
        return false;
    }
    if (playerRemote)
    {
        px = player_get_center_x(playerRemote);
        py = player_get_center_y(playerRemote);
        dx = x - px;
        dy = y - py;
        if (sqrtf(dx * dx + dy * dy) < MIN_DISTANCE)
        {
            return false;
        }
    }
    return true;
}

static void restart_game(SDL_Renderer *renderer, SDL_Window *window, TTF_Font *uiFont, SoundSystem *audio)
{
    // Clean up resources
    SoundSystem_create(audio);
    TTF_CloseFont(uiFont);
    if (tex_player)
        SDL_DestroyTexture(tex_player);
    if (tex_mob)
        SDL_DestroyTexture(tex_mob);
    if (tex_tiles)
        SDL_DestroyTexture(tex_tiles);
    if (tex_extralife)
        SDL_DestroyTexture(tex_extralife);
    if (tex_extraspeed)
        SDL_DestroyTexture(tex_extraspeed);
    if (tex_doubledamage)
        SDL_DestroyTexture(tex_doubledamage);
    if (tex_freezeenemies)
        SDL_DestroyTexture(tex_freezeenemies);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    // Launch new instance
    system("start Game.exe");
    exit(0);
}

int run_game(SDL_Renderer *renderer, SDL_Window *window, TTF_Font *uiFont, SoundSystem *audio)
{
    // Initialize game mode
    NetMode mode = MODE_HOST; // Default to HOST
    char *server_ip = NULL;

    if (!skipMenu)
    {
        int menuResult = showMenu(renderer, window);
        if (menuResult == -1)
        {
            return 0; // Exit Game
        }
        if (menuResult == 1)
        {
            // Multiplayer
            mode = show_multiplayer_menu(renderer, uiFont);
            if (mode == MODE_JOIN)
            {
                server_ip = prompt_for_ip(renderer, uiFont);
            }
        }
    }

    // Initialize network
    if (network_init(mode, server_ip) < 0)
    {
        SDL_Log("Network init failed");
        if (server_ip)
            free(server_ip);
        return 1;
    }
    if (mode == MODE_HOST)
    {
        wait_for_client(renderer, uiFont);
    }

    // Initialize players
    Player *playerLocal = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                        PLAYER_SIZE, DEFAULT_PLAYER_SPEED,
                                        DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    Player *playerRemote = create_player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
                                         PLAYER_SIZE, DEFAULT_PLAYER_SPEED,
                                         DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    if (!playerLocal || !playerRemote)
    {
        SDL_Log("Failed to create players");
        if (server_ip)
            free(server_ip);
        network_shutdown();
        if (playerLocal)
            free(playerLocal);
        if (playerRemote)
            free(playerRemote);
        return 1;
    }
    player_set_tint(playerRemote, (SDL_Color){200, 80, 80, 255});

    if (mode == MODE_JOIN)
    {
        network_send(player_get_x(playerLocal), player_get_y(playerLocal), player_get_aim_angle(playerLocal));
    }

    SoundSystem_playMusic(audio, "source/spelmusik.wav");

    // Load textures
    tex_extralife = IMG_LoadTexture(renderer, "resources/extralife.png");
    tex_extraspeed = IMG_LoadTexture(renderer, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(renderer, "resources/doubledamage.png");
    tex_freezeenemies = IMG_LoadTexture(renderer, "resources/freezeenemies.png");
    tex_tiles = IMG_LoadTexture(renderer, "resources/tiles.png");
    tex_player = IMG_LoadTexture(renderer, "resources/hitman1.png");
    tex_mob = IMG_LoadTexture(renderer, "resources/zombie1.png");

    if (!tex_tiles || !tex_player || !tex_mob || !tex_extralife || !tex_extraspeed || !tex_doubledamage || !tex_freezeenemies)
    {
        SDL_Log("Kunde inte ladda in texturer: %s", IMG_GetError());
        network_shutdown();
        if (server_ip)
            free(server_ip);
        free(playerLocal);
        free(playerRemote);
        return 1;
    }

    srand((unsigned int)time(NULL));
    Uint32 start_time = SDL_GetTicks();
    int current_max_mobs = 10;
    int score = 0;

    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS];
    for (int i = 0; i < current_max_mobs; i++)
    {
        int x, y, type, health;
        do
        {
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
        } while (!is_position_valid(x, y, playerLocal, playerRemote));
        type = rand() % 4;
        health = (type == 3) ? 2 : 1;
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

    if (mode == MODE_HOST)
    {
        for (int i = 0; i < current_max_mobs; i++)
        {
            if (mobs[i].active)
            {
                network_send_spawn_mob(i, mobs[i].rect.x, mobs[i].rect.y, mobs[i].type, mobs[i].health);
            }
        }
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            if (powerups[i].active)
            {
                network_send_spawn_powerup(i, powerups[i].rect.x, powerups[i].rect.y, powerups[i].type);
            }
        }
    }

    Uint32 freeze_timer = 0;
    bool running = true;
    SDL_Event event;
    bool spacePressed = false;
    ActiveEffects effects = {false, 0, false, 0};
    Uint32 now, last_mob_spawn_time = 0, last_powerup_spawn_time = 0;

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
                {
                    continue;
                }
                if (player_get_ammo(playerLocal) <= 0)
                {
                    player_start_reload(playerLocal, SDL_GetTicks());
                    continue;
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
                        float dx = (float)mx - center_x;
                        float dy = (float)my - center_y;
                        float length = sqrtf(dx * dx + dy * dy);
                        bullets[i].dx = BULLET_SPEED * (dx / length);
                        bullets[i].dy = BULLET_SPEED * (dy / length);
                        network_send_fire_bullet(i, bullets[i].rect.x, bullets[i].rect.y, bullets[i].dx, bullets[i].dy);
                        SoundSystem_playEffect(audio, SOUND_SHOOT);
                        break;
                    }
                }
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        extern UDPpacket *pktIn;
        extern UDPsocket netSocket;
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
                {
                    mobs[idx] = create_mob(x, y, MOB_SIZE, type, health);
                }
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
                {
                    powerups[idx] = create_powerup(ptype, x, y);
                }
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
                if (idx >= 0 && idx < MAX_BULLETS)
                {
                    bullets[idx].rect.x = x;
                    bullets[idx].rect.y = y;
                    bullets[idx].dx = dx;
                    bullets[idx].dy = dy;
                    bullets[idx].rect.w = BULLET_SIZE;
                    bullets[idx].rect.h = BULLET_SIZE;
                    bullets[idx].active = true;
                }
            }
            else if (msg == MSG_REMOVE_PWR)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < MAX_POWERUPS)
                {
                    powerups[idx].active = false;
                }
            }
            else if (msg == MSG_REMOVE_MOB)
            {
                int idx;
                memcpy(&idx, pktIn->data + 1, sizeof(int));
                if (idx >= 0 && idx < current_max_mobs)
                {
                    mobs[idx].active = false;
                }
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
                player_set_lives(playerLocal, player_get_lives(playerLocal) - amount);
                player_set_lives(playerRemote, player_get_lives(playerRemote) - amount);
                network_send_set_hp(player_get_lives(playerLocal));
                if (player_get_lives(playerLocal) <= 0)
                {
                    network_send_game_over();
                    int result = showGameOver(renderer);
                    if (result == 0)
                    {
                        network_shutdown();
                        restart_game(renderer, window, uiFont, audio);
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
                player_set_lives(playerLocal, hp);
                player_set_lives(playerRemote, hp);
                if (player_get_lives(playerLocal) <= 0)
                {
                    network_send_game_over();
                    int result = showGameOver(renderer);
                    if (result == 0)
                    {
                        network_shutdown();
                        restart_game(renderer, window, uiFont, audio);
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
                    network_shutdown();
                    restart_game(renderer, window, uiFont, audio);
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
                {
                    bullets[idx].active = false;
                }
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

        if (state[SDL_SCANCODE_SPACE] && !spacePressed)
        {
            if (!player_is_reloading(playerLocal) && player_get_ammo(playerLocal) > 0)
            {
                player_set_ammo(playerLocal, player_get_ammo(playerLocal) - 1);
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
                        float dx = (float)mx - px;
                        float dy = (float)my - py;
                        float length = sqrtf(dx * dx + dy * dy);
                        bullets[i].dx = BULLET_SPEED * (dx / length);
                        bullets[i].dy = BULLET_SPEED * (dy / length);
                        network_send_fire_bullet(i, bullets[i].rect.x, bullets[i].rect.y, bullets[i].dx, bullets[i].dy);
                        SoundSystem_playEffect(audio, SOUND_SHOOT);
                        break;
                    }
                }
            }
            spacePressed = true;
        }
        else if (!state[SDL_SCANCODE_SPACE])
        {
            spacePressed = false;
        }

        if (mode == MODE_HOST)
        {
            for (int i = 0; i < current_max_mobs; i++)
            {
                if (!mobs[i].active)
                {
                    continue;
                }
                if (mobs[i].attacking && SDL_GetTicks() - mobs[i].last_attack_time >= 2000)
                {
                    Uint32 now = SDL_GetTicks();
                    if (now - mobs[i].last_attack_time >= mobs[i].attack_interval)
                    {
                        int damage = (mobs[i].type == 3) ? 2 : 1;
                        player_set_lives(playerLocal, player_get_lives(playerLocal) - damage);
                        player_set_lives(playerRemote, player_get_lives(playerRemote) - damage);
                        network_send_damage(damage);
                        mobs[i].last_attack_time = now;
                        if (player_get_lives(playerLocal) <= 0)
                        {
                            network_send_game_over();
                            int result = showGameOver(renderer);
                            if (result == 0)
                            {
                                network_shutdown();
                                restart_game(renderer, window, uiFont, audio);
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
                            score += 100;
                            if (rand() % 4 == 0)
                            {
                                for (int k = 0; k < MAX_POWERUPS; k++)
                                {
                                    if (!powerups[k].active && !powerups[k].picked_up)
                                    {
                                        PowerupType t = rand() % 4;
                                        powerups[k] = create_powerup(t, mobs[j].rect.x, mobs[j].rect.y);
                                        if (mode == MODE_HOST)
                                        {
                                            network_send_spawn_powerup(k, powerups[k].rect.x, powerups[k].rect.y, t);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        bullets[i].active = false;
                        network_send_remove_bullet(i);
                        break;
                    }
                }
            }
        }

        bool freeze_active = effects.freeze_active && (SDL_GetTicks() - effects.freeze_start_time < 5000);
        for (int i = 0; i < current_max_mobs; i++)
        {
            if (!mobs[i].active || freeze_active)
            {
                continue;
            }
            SDL_Rect rectL = player_get_rect(playerLocal);
            SDL_Rect rectR = player_get_rect(playerRemote);
            float dxL = (rectL.x + rectL.w / 2.0f) - (mobs[i].rect.x + mobs[i].rect.w / 2.0f);
            float dyL = (rectL.y + rectL.h / 2.0f) - (mobs[i].rect.y + mobs[i].rect.h / 2.0f);
            float distL = sqrtf(dxL * dxL + dyL * dyL);
            float dxR = (rectR.x + rectR.w / 2.0f) - (mobs[i].rect.x + mobs[i].rect.w / 2.0f);
            float dyR = (rectR.y + rectR.h / 2.0f) - (mobs[i].rect.y + mobs[i].rect.h / 2.0f);
            float distR = sqrtf(dxR * dxR + dyR * dyR);
            SDL_Rect target = (distL < distR) ? rectL : rectR;
            update_mob(&mobs[i], target);
        }

        now = SDL_GetTicks();
        Uint32 elapsed = (now - start_time) / 10000;
        int new_max = 10 + elapsed * 10;
        if (new_max > MAX_MOBS)
        {
            new_max = MAX_MOBS;
        }
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
                    do
                    {
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
                    } while (!is_position_valid(x, y, playerLocal, playerRemote));
                    type = rand() % 4;
                    health = (type == 3) ? 2 : 1;
                    mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
                    network_send_spawn_mob(i, x, y, type, health);
                    spawned++;
                }
            }
            last_mob_spawn_time = now;
        }

        if (mode == MODE_HOST && now - last_powerup_spawn_time >= 5000)
        {
            for (int i = 0; i < MAX_POWERUPS; i++)
            {
                if (!powerups[i].active && !powerups[i].picked_up)
                {
                    PowerupType t = rand() % 4;
                    int x = rand() % (SCREEN_WIDTH - 32);
                    int y = rand() % (SCREEN_HEIGHT - 32);
                    powerups[i] = create_powerup(t, x, y);
                    network_send_spawn_powerup(i, x, y, t);
                    break;
                }
            }
            last_powerup_spawn_time = now;
        }

        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            check_powerup_collision(&powerups[i], playerLocal, now, &effects);
            if (powerups[i].picked_up && !powerups[i].sound_played)
            {
                powerups[i].active = false;
                network_send_remove_powerup(i);
                if (powerups[i].type == POWERUP_FREEZE_ENEMIES)
                {
                    network_send_freeze();
                }
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
                }
                powerups[i].sound_played = true;
            }
        }
        update_effects(&effects, playerLocal, now, powerups);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        int TILE_SIZE = 32;
        SDL_Rect src = {0, 0, TILE_SIZE, TILE_SIZE};
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
        SDL_RenderCopyEx(renderer, tex_player, NULL, &rect, player_get_aim_angle(playerLocal), &center, SDL_FLIP_NONE);
        SDL_Rect rectRemote = player_get_rect(playerRemote);
        SDL_RenderCopyEx(renderer, tex_player, NULL, &rectRemote, player_get_aim_angle(playerRemote), &center, SDL_FLIP_NONE);
        draw_powerup_bars(renderer, playerLocal, powerups, now);

        for (int i = 0; i < current_max_mobs; i++)
        {
            if (mobs[i].active)
            {
                draw_mob(renderer, &mobs[i], player_get_rect(playerLocal));
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

        int barX = 10, barY = 10, barW = HEALTH_BAR_WIDTH, barH = HEALTH_BAR_HEIGHT;
        SDL_Rect bg = {barX, barY, barW, barH};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &bg);
        int lives = player_get_lives(playerLocal);
        float frac = (float)lives / (float)MAX_HEALTH;
        SDL_Rect fg = {barX, barY, (int)(barW * frac), barH};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &fg);

        char score_text[32];
        sprintf(score_text, "Score: %d", score);
        render_text(renderer, uiFont, score_text, 10, 40);

        char ammo_text[32];
        int ammo = player_get_ammo(playerLocal);
        sprintf(ammo_text, "%d/30", ammo);
        int text_width, text_height;
        TTF_SizeText(uiFont, ammo_text, &text_width, &text_height);
        int x = SCREEN_WIDTH - text_width - 10;
        int y = SCREEN_HEIGHT - text_height - 10;
        render_text(renderer, uiFont, ammo_text, x, y);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    network_shutdown();
    if (server_ip)
    {
        free(server_ip);
    }
    free(playerLocal);
    free(playerRemote);
    return 0;
}