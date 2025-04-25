#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "player.h"
#include "mob.h"
#include "powerups.h"
#include "sound.h"

// Skärm- och spelkonstanter
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 30
#define MOB_SIZE 30
#define BULLET_SIZE 7
#define BULLET_SPEED 7
#define MAX_BULLETS 100
#define MAX_MOBS 5
#define MAX_POWERUPS 5
#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20
#define UDP_PORT 12345
#define CONNECTION_TIMEOUT 20000 // 20 seconds timeout

bool skipMenu = false;
SDL_Texture *tex_player = NULL;
SDL_Texture *tex_mob = NULL;
SDL_Texture *tex_tiles = NULL;
SDL_Texture *tex_extralife = NULL;
SDL_Texture *tex_extraspeed = NULL;
SDL_Texture *tex_doubledamage = NULL;
SDL_Texture *tex_freezeenemies = NULL;
bool is_multiplayer = false;
bool is_host = false;
int local_player_id = 0;

// Bullet-ADT
typedef struct
{
    SDL_Rect rect;
    bool active;
    float dx, dy;
    int owner; // 0 = local player, 1 = remote player
} Bullet;

// Network packet types
typedef enum
{
    PACKET_HANDSHAKE,
    PACKET_PLAYER_POS,
    PACKET_BULLET,
    PACKET_MOB_UPDATE,
    PACKET_POWERUP_UPDATE,
    PACKET_GAME_STATE
} PacketType;

typedef struct
{
    PacketType type;
    int id; // Player ID (0 or 1)
    union
    {
        struct
        {
            int handshake; // 0 = request, 1 = acknowledgment
        } handshake;
        struct
        {
            float x, y;
            float angle; // Player facing angle
        } pos;
        struct
        {
            float x, y, dx, dy;
            bool active;
        } bullet;
        struct
        {
            int index;
            float x, y;
            int w, h; // Added for mob size
            bool active;
            int health;
            int type;
        } mob;
        struct
        {
            int index;
            bool active;
            bool picked_up;
            PowerupType type;
            float x, y;
        } powerup;
        struct
        {
            int lives;
            int score;
            float speed;
            int damage;
        } state;
    };
} Packet;

// Function declarations
int showMenu(SDL_Renderer *renderer, SDL_Window *window);
int showMultiplayerMenu(SDL_Renderer *renderer, SDL_Window *window);
int showSettings(SDL_Renderer *renderer, SDL_Window *window);
int showGameOver(SDL_Renderer *renderer);
int showIPInputScreen(SDL_Renderer *renderer, char *ip_buffer, int buffer_size);
int showConnectionScreen(SDL_Renderer *renderer, UDPsocket socket, IPaddress *peer, bool is_host);
void send_packet(UDPsocket socket, IPaddress peer, Packet *packet);
void handle_network(UDPsocket socket, Player *local_player, int *local_score, Player *remote_player, Bullet bullets[], Mob mobs[], Powerup powerups[], int *remote_lives, int *remote_score, float *remote_speed, int *remote_damage, ActiveEffects *local_effects, ActiveEffects *remote_effects);

bool is_valid_ip(const char *ip);

int main(int argc, char *argv[])
{
    UDPsocket udp_socket = NULL;
    IPaddress peer;

    if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDLNet_Init() < 0)
    {
        SDL_Log("Init error: %s", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) || TTF_Init() == -1)
    {
        SDL_Log("SDL_image/TTF init error: %s", IMG_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Zombie Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
        return 1;

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
        return 1;

    init_sound();
    play_music("source/spelmusik.wav");

    // Menu handling
    int menuResult = skipMenu ? 0 : showMenu(renderer, window);
    if (menuResult == 0)
    {
        is_multiplayer = false;
        is_host = true;
        local_player_id = 0;
    }
    else if (menuResult == 1)
    {
        is_multiplayer = true;
        int mpResult = showMultiplayerMenu(renderer, window);
        if (mpResult == 0) // Host Game
        {
            is_host = true;
            local_player_id = 0;
            SDLNet_ResolveHost(&peer, NULL, UDP_PORT);
            SDL_Log("Hosting game on port %d (0x%08X:%d)", UDP_PORT, peer.host, peer.port);
        }
        else if (mpResult == 1) // Join Game
        {
            is_host = false;
            local_player_id = 1;
            char peer_ip[16] = {0};
            if (showIPInputScreen(renderer, peer_ip, sizeof(peer_ip)) != 0)
            {
                SDL_Log("IP input cancelled or failed");
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                TTF_Quit();
                IMG_Quit();
                SDL_Quit();
                return 0;
            }
            if (!is_valid_ip(peer_ip))
            {
                SDL_Log("Invalid IP address: %s", peer_ip);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                TTF_Quit();
                IMG_Quit();
                SDL_Quit();
                return 0;
            }
            if (SDLNet_ResolveHost(&peer, peer_ip, UDP_PORT) != 0)
            {
                SDL_Log("Failed to resolve host %s:%d: %s", peer_ip, UDP_PORT, SDLNet_GetError());
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                TTF_Quit();
                IMG_Quit();
                SDL_Quit();
                return 0;
            }
            SDL_Log("Attempting to join host at %s:%d (0x%08X:%d)", peer_ip, UDP_PORT, peer.host, peer.port);
        }
        else // Back or Quit
        {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 0;
        }
        udp_socket = SDLNet_UDP_Open(is_host ? UDP_PORT : 0);
        if (!udp_socket)
        {
            SDL_Log("UDP socket error: %s", SDLNet_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        SDL_Log("Opened UDP socket on port %d", is_host ? UDP_PORT : 0);
        if (showConnectionScreen(renderer, udp_socket, &peer, is_host) != 0)
        {
            SDL_Log("Failed to establish connection");
            SDLNet_UDP_Close(udp_socket);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }
    else if (menuResult == -1)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    // Load textures
    tex_player = IMG_LoadTexture(renderer, "resources/hitman1.png");
    tex_mob = IMG_LoadTexture(renderer, "resources/zombie1.png");
    tex_tiles = IMG_LoadTexture(renderer, "resources/tiles.png");
    tex_extralife = IMG_LoadTexture(renderer, "resources/extralife.png");
    tex_extraspeed = IMG_LoadTexture(renderer, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(renderer, "resources/doubledamage.png");
    tex_freezeenemies = IMG_LoadTexture(renderer, "resources/freezeenemies.png");

    if (!tex_player || !tex_mob || !tex_tiles || !tex_extralife || !tex_extraspeed || !tex_doubledamage || !tex_freezeenemies)
    {
        SDL_Log("Texture load error: %s", IMG_GetError());
        return 1;
    }

    srand((unsigned int)time(NULL));

    // Initialize players
    Player local_player = create_player(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2, PLAYER_SIZE, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    Player remote_player = create_player(SCREEN_WIDTH / 2 + 50, SCREEN_HEIGHT / 2, PLAYER_SIZE, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    int local_score = 0, remote_score = 0;
    float remote_speed = DEFAULT_PLAYER_SPEED;
    int remote_damage = DEFAULT_PLAYER_DAMAGE;

    Bullet bullets[MAX_BULLETS] = {0};
    Mob mobs[MAX_MOBS] = {0};
    Powerup powerups[MAX_POWERUPS] = {0};

    if (is_host)
    {
        for (int i = 0; i < MAX_MOBS; i++)
        {
            int x, y, type = rand() % 4, health = (type == 3) ? 2 : 1;
            do
            {
                x = rand() % (SCREEN_WIDTH - MOB_SIZE);
                y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
                mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
            } while (check_mob_collision(&mobs[i].rect, mobs, i));
            if (is_multiplayer)
            {
                Packet m_packet = {PACKET_MOB_UPDATE, local_player_id, .mob = {i, (float)mobs[i].rect.x, (float)mobs[i].rect.y, mobs[i].rect.w, mobs[i].rect.h, mobs[i].active, mobs[i].health, mobs[i].type}};
                send_packet(udp_socket, peer, &m_packet);
            }
        }
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            powerups[i] = create_powerup(rand() % 4, rand() % (SCREEN_WIDTH - 32), rand() % (SCREEN_HEIGHT - 32));
            if (is_multiplayer)
            {
                Packet p_packet = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {i, powerups[i].active, powerups[i].picked_up, powerups[i].type, (float)powerups[i].rect.x, (float)powerups[i].rect.y}};
                send_packet(udp_socket, peer, &p_packet);
            }
        }
    }
    else
    {
        for (int i = 0; i < MAX_MOBS; i++)
        {
            mobs[i].active = false;
        }
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            powerups[i].active = false;
            powerups[i].picked_up = false;
        }
    }

    bool running = true;
    SDL_Event event;
    bool spacePressed = false;
    ActiveEffects local_effects = {0};
    ActiveEffects remote_effects = {0};
    Uint32 now, last_mob_spawn_time = 0, last_powerup_spawn_time = 0;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !spacePressed)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active)
                    {
                        bullets[i].rect.x = local_player.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.y = local_player.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;
                        bullets[i].owner = local_player_id;
                        float dx = (float)mx - (local_player.rect.x + PLAYER_SIZE / 2);
                        float dy = (float)my - (local_player.rect.y + PLAYER_SIZE / 2);
                        float length = sqrtf(dx * dx + dy * dy);
                        if (length > 0)
                        {
                            bullets[i].dx = BULLET_SPEED * (dx / length);
                            bullets[i].dy = BULLET_SPEED * (dy / length);
                        }
                        else
                        {
                            bullets[i].dx = BULLET_SPEED;
                            bullets[i].dy = 0;
                        }
                        play_sound(SOUND_SHOOT);
                        if (is_multiplayer)
                        {
                            Packet packet = {PACKET_BULLET, local_player_id, .bullet = {(float)bullets[i].rect.x, (float)bullets[i].rect.y, bullets[i].dx, bullets[i].dy, bullets[i].active}};
                            send_packet(udp_socket, peer, &packet);
                        }
                        break;
                    }
                }
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDL_SCANCODE_SPACE && !spacePressed)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active)
                    {
                        bullets[i].rect.x = local_player.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.y = local_player.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].active = true;
                        bullets[i].owner = local_player_id;
                        float dx = (float)mx - (local_player.rect.x + PLAYER_SIZE / 2);
                        float dy = (float)my - (local_player.rect.y + PLAYER_SIZE / 2);
                        float length = sqrtf(dx * dx + dy * dy);
                        if (length > 0)
                        {
                            bullets[i].dx = BULLET_SPEED * (dx / length);
                            bullets[i].dy = BULLET_SPEED * (dy / length);
                        }
                        else
                        {
                            bullets[i].dx = BULLET_SPEED;
                            bullets[i].dy = 0;
                        }
                        play_sound(SOUND_SHOOT);
                        if (is_multiplayer)
                        {
                            Packet packet = {PACKET_BULLET, local_player_id, .bullet = {(float)bullets[i].rect.x, (float)bullets[i].rect.y, bullets[i].dx, bullets[i].dy, bullets[i].active}};
                            send_packet(udp_socket, peer, &packet);
                        }
                        spacePressed = true;
                        break;
                    }
                }
            }
            if (event.type == SDL_KEYUP && event.key.keysym.sym == SDL_SCANCODE_SPACE)
            {
                spacePressed = false;
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        update_player(&local_player, state);

        if (is_multiplayer)
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            float dx = (float)mx - (local_player.rect.x + PLAYER_SIZE / 2);
            float dy = (float)my - (local_player.rect.y + PLAYER_SIZE / 2);
            float angle = atan2f(dy, dx) * 180.0f / M_PI;
            Packet pos_packet = {PACKET_PLAYER_POS, local_player_id, .pos = {(float)local_player.rect.x, (float)local_player.rect.y, angle}};
            send_packet(udp_socket, peer, &pos_packet);
            handle_network(udp_socket, &local_player, &local_score, &remote_player, bullets, mobs, powerups, &remote_player.lives, &remote_score, &remote_speed, &remote_damage, &local_effects, &remote_effects);
        }

        now = SDL_GetTicks();
        if (is_host)
        {
            for (int i = 0; i < MAX_MOBS; i++)
            {
                if (mobs[i].active)
                {
                    if (!local_effects.freeze_active && !remote_effects.freeze_active)
                        update_mob(&mobs[i], local_player.rect, remote_player.rect, is_multiplayer);
                    if (mobs[i].attacking && now - mobs[i].last_attack_time >= mobs[i].attack_interval)
                    {
                        float dx_local = (local_player.rect.x + local_player.rect.w / 2) - (mobs[i].rect.x + mobs[i].rect.w / 2);
                        float dy_local = (local_player.rect.y + local_player.rect.h / 2) - (mobs[i].rect.y + mobs[i].rect.h / 2);
                        float dist_local = sqrtf(dx_local * dx_local + dy_local * dy_local);

                        float dx_remote = (remote_player.rect.x + remote_player.rect.w / 2) - (mobs[i].rect.x + mobs[i].rect.w / 2);
                        float dy_remote = (remote_player.rect.y + remote_player.rect.h / 2) - (mobs[i].rect.y + mobs[i].rect.h / 2);
                        float dist_remote = sqrtf(dx_remote * dx_remote + dy_remote * dy_remote);

                        if (is_multiplayer && dist_remote < dist_local)
                        {
                            remote_player.lives -= (mobs[i].type == 3) ? 2 : 1;
                            if (remote_player.lives < 0)
                                remote_player.lives = 0;
                        }
                        else
                        {
                            local_player.lives -= (mobs[i].type == 3) ? 2 : 1;
                            if (local_player.lives < 0)
                                local_player.lives = 0;
                        }
                        mobs[i].last_attack_time = now;
                        if (is_multiplayer)
                        {
                            Packet state_packet_local = {PACKET_GAME_STATE, 0, .state = {local_player.lives, local_score, local_player.speed, local_player.damage}};
                            send_packet(udp_socket, peer, &state_packet_local);
                            Packet state_packet_remote = {PACKET_GAME_STATE, 1, .state = {remote_player.lives, remote_score, remote_player.speed, remote_player.damage}};
                            send_packet(udp_socket, peer, &state_packet_remote);
                        }
                    }
                    if (is_multiplayer)
                    {
                        Packet m_packet = {PACKET_MOB_UPDATE, local_player_id, .mob = {i, (float)mobs[i].rect.x, (float)mobs[i].rect.y, mobs[i].rect.w, mobs[i].rect.h, mobs[i].active, mobs[i].health, mobs[i].type}};
                        send_packet(udp_socket, peer, &m_packet);
                    }
                }
            }

            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    for (int j = 0; j < MAX_MOBS; j++)
                    {
                        if (mobs[j].active && SDL_HasIntersection(&bullets[i].rect, &mobs[j].rect))
                        {
                            mobs[j].health -= (bullets[i].owner == local_player_id) ? local_player.damage : remote_player.damage;
                            if (mobs[j].health <= 0)
                            {
                                mobs[j].active = false;
                                if (bullets[i].owner == local_player_id)
                                    local_score += 100;
                                else
                                    remote_score += 100;
                                if (rand() % 2 == 0)
                                {
                                    for (int k = 0; k < MAX_POWERUPS; k++)
                                    {
                                        if (!powerups[k].active && !powerups[k].picked_up)
                                        {
                                            powerups[k] = create_powerup(rand() % 4, mobs[j].rect.x, mobs[j].rect.y);
                                            if (is_multiplayer)
                                            {
                                                Packet p_packet = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {k, powerups[k].active, powerups[k].picked_up, powerups[k].type, (float)powerups[k].rect.x, (float)powerups[k].rect.y}};
                                                send_packet(udp_socket, peer, &p_packet);
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            bullets[i].active = false;
                            if (is_multiplayer)
                            {
                                Packet m_packet = {PACKET_MOB_UPDATE, local_player_id, .mob = {j, (float)mobs[j].rect.x, (float)mobs[j].rect.y, mobs[j].rect.w, mobs[j].rect.h, mobs[j].active, mobs[j].health, mobs[j].type}};
                                send_packet(udp_socket, peer, &m_packet);
                                Packet b_packet = {PACKET_BULLET, local_player_id, .bullet = {(float)bullets[i].rect.x, (float)bullets[i].rect.y, bullets[i].dx, bullets[i].dy, bullets[i].active}};
                                send_packet(udp_socket, peer, &b_packet);
                            }
                            SDL_Log("Host: Bullet %d hit mob %d, mob health=%d, bullet active=%d", i, j, mobs[j].health, bullets[i].active);
                            break;
                        }
                    }
                }
            }

            if (now - last_mob_spawn_time >= 1500)
            {
                for (int i = 0; i < MAX_MOBS; i++)
                {
                    if (!mobs[i].active)
                    {
                        int x, y, type = rand() % 4, health = (type == 3) ? 2 : 1;
                        do
                        {
                            x = rand() % (SCREEN_WIDTH - MOB_SIZE);
                            y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
                            mobs[i] = create_mob(x, y, MOB_SIZE, type, health);
                        } while (check_mob_collision(&mobs[i].rect, mobs, i));
                        if (is_multiplayer)
                        {
                            Packet m_packet = {PACKET_MOB_UPDATE, local_player_id, .mob = {i, (float)mobs[i].rect.x, (float)mobs[i].rect.y, mobs[i].rect.w, mobs[i].rect.h, mobs[i].active, mobs[i].health, mobs[i].type}};
                            send_packet(udp_socket, peer, &m_packet);
                        }
                        break;
                    }
                }
                last_mob_spawn_time = now;
            }

            if (now - last_powerup_spawn_time >= 5000)
            {
                for (int i = 0; i < MAX_POWERUPS; i++)
                {
                    if (!powerups[i].active || (powerups[i].picked_up && now - powerups[i].pickup_time >= powerups[i].duration))
                    {
                        powerups[i].active = false;
                        powerups[i].picked_up = false;
                        powerups[i] = create_powerup(rand() % 4, rand() % (SCREEN_WIDTH - 32), rand() % (SCREEN_HEIGHT - 32));
                        if (is_multiplayer)
                        {
                            Packet p_packet = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {i, powerups[i].active, powerups[i].picked_up, powerups[i].type, (float)powerups[i].rect.x, (float)powerups[i].rect.y}};
                            send_packet(udp_socket, peer, &p_packet);
                        }
                        break;
                    }
                }
                last_powerup_spawn_time = now;
            }

            // Send periodic state updates
            if (is_multiplayer)
            {
                Packet state_packet_local = {PACKET_GAME_STATE, 0, .state = {local_player.lives, local_score, local_player.speed, local_player.damage}};
                send_packet(udp_socket, peer, &state_packet_local);
                Packet state_packet_remote = {PACKET_GAME_STATE, 1, .state = {remote_player.lives, remote_score, remote_player.speed, remote_player.damage}};
                send_packet(udp_socket, peer, &state_packet_remote);
            }
        }

        // Powerup collisions for local player (both host and client detect, host applies)
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            if (powerups[i].active && !powerups[i].picked_up)
            {
                bool local_collision = SDL_HasIntersection(&powerups[i].rect, &local_player.rect);
                if (local_collision)
                {
                    powerups[i].picked_up = true; // Mark as picked up locally for both host and client
                    powerups[i].pickup_time = now;
                    if (is_host)
                    {
                        // Host applies the effect immediately for its local player
                        apply_powerup_effect(&powerups[i], &local_player, &local_effects, now);
                        Packet state_packet = {PACKET_GAME_STATE, local_player_id, .state = {local_player.lives, local_score, local_player.speed, local_player.damage}};
                        send_packet(udp_socket, peer, &state_packet);
                    }
                    if (!powerups[i].sound_played)
                    {
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
                        }
                        powerups[i].sound_played = true;
                    }
                    if (is_multiplayer)
                    {
                        // Send update to peer (client notifies host, host broadcasts)
                        Packet p_packet = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {i, powerups[i].active, powerups[i].picked_up, powerups[i].type, (float)powerups[i].rect.x, (float)powerups[i].rect.y}};
                        send_packet(udp_socket, peer, &p_packet);
                    }
                    SDL_Log("Player %d picked up powerup %d (type=%d)", local_player_id, i, powerups[i].type);
                }
            }
        }

        update_effects(&local_effects, &local_player.speed, &local_player.damage, now, powerups);
        if (is_multiplayer)
            update_effects(&remote_effects, &remote_player.speed, &remote_player.damage, now, powerups);

        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                bullets[i].rect.x += (int)bullets[i].dx;
                bullets[i].rect.y += (int)bullets[i].dy;
                if (bullets[i].rect.x < 0 || bullets[i].rect.x > SCREEN_WIDTH || bullets[i].rect.y < 0 || bullets[i].rect.y > SCREEN_HEIGHT)
                {
                    bullets[i].active = false;
                    if (is_multiplayer && is_host)
                    {
                        Packet b_packet = {PACKET_BULLET, local_player_id, .bullet = {(float)bullets[i].rect.x, (float)bullets[i].rect.y, bullets[i].dx, bullets[i].dy, bullets[i].active}};
                        send_packet(udp_socket, peer, &b_packet);
                    }
                }
            }
        }

        if (!is_host)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active && bullets[i].owner == local_player_id)
                {
                    for (int j = 0; j < MAX_MOBS; j++)
                    {
                        if (mobs[j].active && SDL_HasIntersection(&bullets[i].rect, &mobs[j].rect))
                        {
                            bullets[i].active = false;
                            SDL_Log("Client: Bullet %d visually hit mob %d", i, j);
                            break;
                        }
                    }
                }
            }
        }

        if (is_multiplayer)
        {
            remote_player.speed = remote_speed;
            remote_player.damage = remote_damage;
        }

        if (local_player.lives <= 0 || (is_multiplayer && remote_player.lives <= 0))
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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int TILE_SIZE = 32;
        SDL_Rect src = {0, 0, TILE_SIZE, TILE_SIZE};
        SDL_Rect dest;
        for (int y = 0; y < SCREEN_HEIGHT; y += TILE_SIZE)
        {
            for (int x = 0; x < SCREEN_WIDTH; x += TILE_SIZE)
            {
                dest = (SDL_Rect){x, y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, tex_tiles, &src, &dest);
            }
        }

        draw_player(renderer, &local_player);
        if (is_multiplayer)
            draw_player(renderer, &remote_player);
        draw_powerup_bars(renderer, &local_player, powerups, now);
        if (is_multiplayer)
            draw_powerup_bars(renderer, &remote_player, powerups, now);
        for (int i = 0; i < MAX_MOBS; i++)
        {
            if (mobs[i].active)
            {
                draw_mob(renderer, &mobs[i], local_player.rect);
                SDL_Log("Rendering mob %d: active=%d, x=%d, y=%d, w=%d, h=%d, health=%d, type=%d", i, mobs[i].active, mobs[i].rect.x, mobs[i].rect.y, mobs[i].rect.w, mobs[i].rect.h, mobs[i].health, mobs[i].type);
            }
        }
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                SDL_SetRenderDrawColor(renderer, bullets[i].owner == local_player_id ? 255 : 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &bullets[i].rect);
                SDL_Log("Rendering bullet %d: active=%d, x=%d, y=%d, owner=%d", i, bullets[i].active, bullets[i].rect.x, bullets[i].rect.y, bullets[i].owner);
            }
        }
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            if (powerups[i].active && !powerups[i].picked_up)
            {
                draw_powerup(renderer, &powerups[i]);
                SDL_Log("Rendering powerup %d: active=%d, picked_up=%d, x=%d, y=%d, type=%d", i, powerups[i].active, powerups[i].picked_up, powerups[i].rect.x, powerups[i].rect.y, powerups[i].type);
            }
        }

        SDL_Rect healthBarBg = {10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBarBg);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect healthBar = {10, 10, (HEALTH_BAR_WIDTH * local_player.lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(renderer, &healthBar);

        if (is_multiplayer)
        {
            SDL_Rect remoteHealthBarBg = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &remoteHealthBarBg);
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect remoteHealthBar = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, (HEALTH_BAR_WIDTH * remote_player.lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
            SDL_RenderFillRect(renderer, &remoteHealthBar);
        }

        printf("Local Score: %d | Lives: %d | Remote Score: %d | Lives: %d\n", local_score, local_player.lives, remote_score, remote_player.lives);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (is_multiplayer && udp_socket)
    {
        SDLNet_UDP_Close(udp_socket);
        SDLNet_Quit();
    }
    SDL_DestroyTexture(tex_player);
    SDL_DestroyTexture(tex_mob);
    SDL_DestroyTexture(tex_tiles);
    SDL_DestroyTexture(tex_extralife);
    SDL_DestroyTexture(tex_extraspeed);
    SDL_DestroyTexture(tex_doubledamage);
    SDL_DestroyTexture(tex_freezeenemies);
    cleanup_sound();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}

bool is_valid_ip(const char *ip)
{
    int dots = 0, digits = 0;
    for (int i = 0; ip[i]; i++)
    {
        if (ip[i] == '.')
            dots++;
        else if (isdigit(ip[i]))
            digits++;
        else
            return false;
    }
    return dots == 3 && digits >= 4;
}

void send_packet(UDPsocket socket, IPaddress peer, Packet *packet)
{
    UDPpacket *udp_packet = SDLNet_AllocPacket(512);
    if (!udp_packet)
    {
        SDL_Log("Failed to allocate UDP packet");
        return;
    }
    memcpy(udp_packet->data, packet, sizeof(Packet));
    udp_packet->len = sizeof(Packet);
    udp_packet->address = peer;
    if (SDLNet_UDP_Send(socket, -1, udp_packet) == 0)
    {
        SDL_Log("Failed to send packet type %d from player %d: %s", packet->type, packet->id, SDLNet_GetError());
    }
    else
    {
        SDL_Log("Sent packet type %d from player %d to 0x%08X:%d", packet->type, packet->id, peer.host, peer.port);
    }
    SDLNet_FreePacket(udp_packet);
}

void handle_network(UDPsocket socket, Player *local_player, int *local_score, Player *remote_player, Bullet bullets[], Mob mobs[], Powerup powerups[], int *remote_lives, int *remote_score, float *remote_speed, int *remote_damage, ActiveEffects *local_effects, ActiveEffects *remote_effects)
{
    UDPpacket *packet = SDLNet_AllocPacket(512);
    if (!packet)
    {
        SDL_Log("Failed to allocate packet for receiving");
        return;
    }
    while (SDLNet_UDP_Recv(socket, packet))
    {
        Packet *p = (Packet *)packet->data;
        if (p->id != local_player_id || p->type == PACKET_GAME_STATE) // Process all GAME_STATE packets
        {
            SDL_Log("Received packet type %d from player %d at 0x%08X:%d", p->type, p->id, packet->address.host, packet->address.port);
            switch (p->type)
            {
            case PACKET_HANDSHAKE:
                SDL_Log("Received handshake from player %d", p->id);
                break;
            case PACKET_PLAYER_POS:
                remote_player->rect.x = (int)p->pos.x;
                remote_player->rect.y = (int)p->pos.y;
                remote_player->angle = p->pos.angle;
                SDL_Log("Updated remote player: x=%d, y=%d, angle=%.2f", remote_player->rect.x, remote_player->rect.y, remote_player->angle);
                break;
            case PACKET_BULLET:
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active || (bullets[i].rect.x == (int)p->bullet.x && bullets[i].rect.y == (int)p->bullet.y))
                    {
                        bullets[i].rect.x = (int)p->bullet.x;
                        bullets[i].rect.y = (int)p->bullet.y;
                        bullets[i].rect.w = BULLET_SIZE;
                        bullets[i].rect.h = BULLET_SIZE;
                        bullets[i].dx = p->bullet.dx;
                        bullets[i].dy = p->bullet.dy;
                        bullets[i].active = p->bullet.active;
                        bullets[i].owner = p->id;
                        SDL_Log("Updated bullet %d: x=%d, y=%d, dx=%.2f, dy=%.2f, active=%d, owner=%d", i, bullets[i].rect.x, bullets[i].rect.y, bullets[i].dx, bullets[i].dy, bullets[i].active, bullets[i].owner);
                        break;
                    }
                }
                break;
            case PACKET_MOB_UPDATE:
                if (!is_host && p->mob.index >= 0 && p->mob.index < MAX_MOBS)
                {
                    mobs[p->mob.index].rect.x = (int)p->mob.x;
                    mobs[p->mob.index].rect.y = (int)p->mob.y;
                    mobs[p->mob.index].rect.w = p->mob.w;
                    mobs[p->mob.index].rect.h = p->mob.h;
                    mobs[p->mob.index].active = p->mob.active;
                    mobs[p->mob.index].health = p->mob.health;
                    mobs[p->mob.index].type = p->mob.type;
                    SDL_Log("Updated mob %d: active=%d, x=%d, y=%d, w=%d, h=%d, health=%d, type=%d", p->mob.index, mobs[p->mob.index].active, mobs[p->mob.index].rect.x, mobs[p->mob.index].rect.y, mobs[p->mob.index].rect.w, mobs[p->mob.index].rect.h, mobs[p->mob.index].health, mobs[p->mob.index].type);
                }
                break;
            case PACKET_POWERUP_UPDATE:
                if (p->powerup.index >= 0 && p->powerup.index < MAX_POWERUPS)
                {
                    // Update powerup state
                    powerups[p->powerup.index].active = p->powerup.active;
                    powerups[p->powerup.index].type = p->powerup.type;
                    powerups[p->powerup.index].rect.x = (int)p->powerup.x;
                    powerups[p->powerup.index].rect.y = (int)p->powerup.y;
                    powerups[p->powerup.index].rect.w = 32;
                    powerups[p->powerup.index].rect.h = 32;

                    // Only proceed if the powerup was not already picked up locally
                    if (p->powerup.picked_up && !powerups[p->powerup.index].picked_up)
                    {
                        powerups[p->powerup.index].picked_up = true;
                        powerups[p->powerup.index].pickup_time = SDL_GetTicks();

                        if (!powerups[p->powerup.index].sound_played)
                        {
                            switch (powerups[p->powerup.index].type)
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
                            }
                            powerups[p->powerup.index].sound_played = true;
                        }

                        if (is_host && p->id == 1)
                        {
                            // Host applies effect for client’s pickup if powerup is still valid
                            apply_powerup_effect(&powerups[p->powerup.index], remote_player, remote_effects, SDL_GetTicks());
                            Packet state_packet = {PACKET_GAME_STATE, 1, .state = {remote_player->lives, *remote_score, remote_player->speed, remote_player->damage}};
                            send_packet(socket, packet->address, &state_packet);
                            // Broadcast the pickup to ensure all clients are synced
                            Packet p_packet = {PACKET_POWERUP_UPDATE, 0, .powerup = {p->powerup.index, powerups[p->powerup.index].active, powerups[p->powerup.index].picked_up, powerups[p->powerup.index].type, (float)powerups[p->powerup.index].rect.x, (float)powerups[p->powerup.index].rect.y}};
                            send_packet(socket, packet->address, &p_packet);
                        }
                    }
                    SDL_Log("Updated powerup %d: active=%d, picked_up=%d, x=%d, y=%d, type=%d", p->powerup.index, powerups[p->powerup.index].active, powerups[p->powerup.index].picked_up, powerups[p->powerup.index].rect.x, powerups[p->powerup.index].rect.y, powerups[p->powerup.index].type);
                }
                break;
            case PACKET_GAME_STATE:
                if (p->id == local_player_id) // Update local player stats
                {
                    local_player->lives = p->state.lives;
                    *local_score = p->state.score;
                    local_player->speed = p->state.speed;
                    local_player->damage = p->state.damage;
                }
                else // Update remote player stats
                {
                    remote_player->lives = p->state.lives;
                    *remote_score = p->state.score;
                    remote_player->speed = p->state.speed;
                    remote_player->damage = p->state.damage;
                }
                break;
            }
        }
    }
    SDLNet_FreePacket(packet);
}

int showMenu(SDL_Renderer *renderer, SDL_Window *window)
{
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char *labels[3] = {"Single Player", "Multiplayer", "Settings"};

    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);
    if (!font || !titleFont)
    {
        SDL_Log("Font load error: %s", TTF_GetError());
        return -1;
    }

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
                    {
                        showSettings(renderer, window);
                    }
                    else
                    {
                        inMenu = false;
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
                    {
                        selected = i;
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        if (i == 2)
                        {
                            showSettings(renderer, window);
                        }
                        else
                        {
                            inMenu = false;
                            return i;
                        }
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont)
        {
            SDL_Color red = {200, 0, 0, 255};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Zombie Game", red);
            if (titleSurface)
            {
                SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
                SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
                SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
                SDL_FreeSurface(titleSurface);
                SDL_DestroyTexture(titleTexture);
            }
        }

        for (int i = 0; i < 3; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color color = {255, 255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(font, labels[i], color);
            if (surface)
            {
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
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(titleFont);
    TTF_CloseFont(font);
    return -1;
}

int showMultiplayerMenu(SDL_Renderer *renderer, SDL_Window *window)
{
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[3];
    const char *labels[3] = {"Host Game", "Join Game", "Back"};

    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);
    if (!font || !titleFont)
    {
        SDL_Log("Font load error: %s", TTF_GetError());
        return -1;
    }

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
                    inMenu = false;
                    return selected;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        selected = i;
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        inMenu = false;
                        return i;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont)
        {
            SDL_Color red = {200, 0, 0, 255};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Multiplayer", red);
            if (titleSurface)
            {
                SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
                SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
                SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
                SDL_FreeSurface(titleSurface);
                SDL_DestroyTexture(titleTexture);
            }
        }

        for (int i = 0; i < 3; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color color = {255, 255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(font, labels[i], color);
            if (surface)
            {
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
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(titleFont);
    TTF_CloseFont(font);
    return -1;
}

int showSettings(SDL_Renderer *renderer, SDL_Window *window)
{
    SDL_Event event;
    bool inSettings = true;
    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 48);
    if (!font || !titleFont)
    {
        SDL_Log("Font load error: %s", TTF_GetError());
        return -1;
    }

    while (inSettings)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return -1;
            if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN))
            {
                inSettings = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Settings", white);
        if (titleSurface)
        {
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 50, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }

        SDL_Surface *volumeSurface = TTF_RenderText_Blended(font, "Volume", white);
        if (volumeSurface)
        {
            SDL_Texture *volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
            SDL_Rect volumeRect = {100, 180, volumeSurface->w, volumeSurface->h};
            SDL_RenderCopy(renderer, volumeTexture, NULL, &volumeRect);
            SDL_FreeSurface(volumeSurface);
            SDL_DestroyTexture(volumeTexture);
        }

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
    if (!optionFont || !titleFont)
    {
        SDL_Log("Font load error: %s", TTF_GetError());
        return -1;
    }

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
                    {
                        selected = i;
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 3; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        return i;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (titleFont)
        {
            SDL_Color red = {200, 0, 0, 255};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "GAME OVER", red);
            if (titleSurface)
            {
                SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
                SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
                SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
                SDL_FreeSurface(titleSurface);
                SDL_DestroyTexture(titleTexture);
            }
        }

        for (int i = 0; i < 3; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);

            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(optionFont, labels[i], white);
            if (surface)
            {
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
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(optionFont);
    TTF_CloseFont(titleFont);
    return -1;
}

int showIPInputScreen(SDL_Renderer *renderer, char *ip_buffer, int buffer_size)
{
    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    if (!font)
    {
        SDL_Log("Font load error: %s", TTF_GetError());
        return -1;
    }

    SDL_Event event;
    bool done = false;
    char input_text[16] = {0};
    int cursor = 0;
    SDL_Rect confirm_button = {SCREEN_WIDTH / 2 - 100, 400, 200, 50};

    SDL_StartTextInput();
    while (!done)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDL_StopTextInput();
                TTF_CloseFont(font);
                return -1;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_RETURN && cursor > 0)
                {
                    if (is_valid_ip(input_text))
                    {
                        strncpy(ip_buffer, input_text, buffer_size);
                        done = true;
                    }
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE && cursor > 0)
                {
                    input_text[--cursor] = '\0';
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    SDL_StopTextInput();
                    TTF_CloseFont(font);
                    return -1;
                }
            }
            if (event.type == SDL_TEXTINPUT && cursor < sizeof(input_text) - 1)
            {
                if (strchr("0123456789.", event.text.text[0]))
                {
                    input_text[cursor++] = event.text.text[0];
                    input_text[cursor] = '\0';
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                if (mx >= confirm_button.x && mx <= confirm_button.x + confirm_button.w &&
                    my >= confirm_button.y && my <= confirm_button.y + confirm_button.h && cursor > 0)
                {
                    if (is_valid_ip(input_text))
                    {
                        strncpy(ip_buffer, input_text, buffer_size);
                        done = true;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *prompt_surface = TTF_RenderText_Blended(font, "Enter Host IP (e.g., 127.0.0.1):", white);
        if (prompt_surface)
        {
            SDL_Texture *prompt_texture = SDL_CreateTextureFromSurface(renderer, prompt_surface);
            SDL_Rect prompt_rect = {SCREEN_WIDTH / 2 - prompt_surface->w / 2, 200, prompt_surface->w, prompt_surface->h};
            SDL_RenderCopy(renderer, prompt_texture, NULL, &prompt_rect);
            SDL_FreeSurface(prompt_surface);
            SDL_DestroyTexture(prompt_texture);
        }

        SDL_Surface *input_surface = TTF_RenderText_Blended(font, input_text[0] ? input_text : " ", white);
        if (input_surface)
        {
            SDL_Texture *input_texture = SDL_CreateTextureFromSurface(renderer, input_surface);
            SDL_Rect input_rect = {SCREEN_WIDTH / 2 - input_surface->w / 2, 250, input_surface->w, input_surface->h};
            SDL_RenderCopy(renderer, input_texture, NULL, &input_rect);
            SDL_FreeSurface(input_surface);
            SDL_DestroyTexture(input_texture);
        }

        SDL_SetRenderDrawColor(renderer, 180, 0, 0, 255);
        SDL_RenderFillRect(renderer, &confirm_button);
        SDL_Surface *confirm_surface = TTF_RenderText_Blended(font, "Confirm", white);
        if (confirm_surface)
        {
            SDL_Texture *confirm_texture = SDL_CreateTextureFromSurface(renderer, confirm_surface);
            SDL_Rect confirm_text_rect = {
                confirm_button.x + (confirm_button.w - confirm_surface->w) / 2,
                confirm_button.y + (confirm_button.h - confirm_surface->h) / 2,
                confirm_surface->w,
                confirm_surface->h};
            SDL_RenderCopy(renderer, confirm_texture, NULL, &confirm_text_rect);
            SDL_FreeSurface(confirm_surface);
            SDL_DestroyTexture(confirm_texture);
        }

        if (cursor > 0 && !is_valid_ip(input_text))
        {
            SDL_Surface *error_surface = TTF_RenderText_Blended(font, "Invalid IP format", white);
            if (error_surface)
            {
                SDL_Texture *error_texture = SDL_CreateTextureFromSurface(renderer, error_surface);
                SDL_Rect error_rect = {SCREEN_WIDTH / 2 - error_surface->w / 2, 300, error_surface->w, error_surface->h};
                SDL_RenderCopy(renderer, error_texture, NULL, &error_rect);
                SDL_FreeSurface(error_surface);
                SDL_DestroyTexture(error_texture);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    return 0;
}

int showConnectionScreen(SDL_Renderer *renderer, UDPsocket socket, IPaddress *peer, bool is_host)
{
    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    if (!font)
    {
        SDL_Log("Font load error: %s", TTF_GetError());
        return -1;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Event event;
    bool connected = false;
    Uint32 start_time = SDL_GetTicks();
    Uint32 last_handshake_time = 0;
    UDPpacket *packet = SDLNet_AllocPacket(512);
    if (!packet)
    {
        SDL_Log("Failed to allocate packet");
        TTF_CloseFont(font);
        return -1;
    }

    const char *error_message = NULL;
    while (!connected && (SDL_GetTicks() - start_time) < CONNECTION_TIMEOUT)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDLNet_FreePacket(packet);
                TTF_CloseFont(font);
                return -1;
            }
        }

        Uint32 now = SDL_GetTicks();
        if (is_host)
        {
            if (SDLNet_UDP_Recv(socket, packet))
            {
                Packet *p = (Packet *)packet->data;
                if (p->type == PACKET_HANDSHAKE && p->id == 1 && p->handshake.handshake == 0)
                {
                    peer->host = packet->address.host;
                    peer->port = packet->address.port;
                    Packet ack = {PACKET_HANDSHAKE, 0, .handshake = {1}};
                    send_packet(socket, *peer, &ack);
                    connected = true;
                    SDL_Log("Connected to client at 0x%08X:%d", peer->host, peer->port);
                }
            }
        }
        else
        {
            if (now - last_handshake_time >= 50)
            {
                Packet handshake = {PACKET_HANDSHAKE, 1, .handshake = {0}};
                send_packet(socket, *peer, &handshake);
                last_handshake_time = now;
            }
            if (SDLNet_UDP_Recv(socket, packet))
            {
                Packet *p = (Packet *)packet->data;
                if (p->type == PACKET_HANDSHAKE && p->id == 0 && p->handshake.handshake == 1)
                {
                    connected = true;
                    SDL_Log("Connected to host at 0x%08X:%d", peer->host, peer->port);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        const char *message = is_host ? "Waiting for client..." : "Connecting to host...";
        SDL_Surface *surface = TTF_RenderText_Blended(font, message, white);
        if (surface)
        {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 - surface->h / 2, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (!connected)
    {
        if (!is_host && peer->host == 0)
            error_message = "Invalid host IP address";
        else
            error_message = "Connection timed out";

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Surface *surface = TTF_RenderText_Blended(font, error_message, white);
        if (surface)
        {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {SCREEN_WIDTH / 2 - surface->w / 2, SCREEN_HEIGHT / 2 - surface->h / 2, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(2000);
        SDL_Log("%s after %d ms", error_message, CONNECTION_TIMEOUT);
    }

    SDLNet_FreePacket(packet);
    TTF_CloseFont(font);
    return connected ? 0 : -1;
}