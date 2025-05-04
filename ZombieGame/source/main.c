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
#include <ctype.h>
#include "player.h"
#include "mob.h"
#include "powerups.h"
#include "sound.h"

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
#define CONNECTION_TIMEOUT 20000

bool skipMenu = false;
SDL_Texture *tex_player, *tex_mob, *tex_tiles, *tex_extralife, *tex_extraspeed, *tex_doubledamage, *tex_freezeenemies;
bool is_multiplayer = false, is_host = false;
int local_player_id = 0;

typedef struct
{
    SDL_Rect rect;
    bool active;
    float dx, dy;
    int owner;
} Bullet;

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
    int id;
    union
    {
        struct
        {
            int handshake;
        } handshake;
        struct
        {
            float x, y, angle;
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
            int w, h;
            bool active;
            int health, type;
        } mob;
        struct
        {
            int index;
            bool active, picked_up;
            PowerupType type;
            float x, y;
        } powerup;
        struct
        {
            int lives, score;
            float speed;
            int damage;
        } state;
    };
} Packet;

// Function to check if an IP address is valid
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

// Function to send a packet over UDP
void send_packet(UDPsocket s, IPaddress peer, Packet *p)
{
    UDPpacket *up = SDLNet_AllocPacket(512);
    if (!up)
        return;
    memcpy(up->data, p, sizeof(Packet));
    up->len = sizeof(Packet);
    up->address = peer;
    SDLNet_UDP_Send(s, -1, up);
    SDLNet_FreePacket(up);
}

int showIPInputScreen(SDL_Renderer *r, char *ip_buffer, int sz)
{
    TTF_Font *f = TTF_OpenFont("shlop.ttf", 28);
    if (!f)
        return -1;
    SDL_Event e;
    bool done = false;
    char input[16] = {0};
    int cursor = 0;
    SDL_Rect cb = {SCREEN_WIDTH / 2 - 100, 400, 200, 50};
    SDL_StartTextInput();
    while (!done)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                SDL_StopTextInput();
                TTF_CloseFont(f);
                return -1;
            }
            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_RETURN && cursor > 0 && is_valid_ip(input))
                {
                    strncpy(ip_buffer, input, sz);
                    done = true;
                }
                else if (e.key.keysym.sym == SDLK_BACKSPACE && cursor > 0)
                {
                    input[--cursor] = '\0';
                }
                else if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    SDL_StopTextInput();
                    TTF_CloseFont(f);
                    return -1;
                }
            }
            if (e.type == SDL_TEXTINPUT && cursor < sizeof(input) - 1 && strchr("0123456789.", e.text.text[0]))
            {
                input[cursor++] = e.text.text[0];
                input[cursor] = '\0';
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x, my = e.button.y;
                if (mx >= cb.x && mx <= cb.x + cb.w && my >= cb.y && my <= cb.y + cb.h && cursor > 0 && is_valid_ip(input))
                {
                    strncpy(ip_buffer, input, sz);
                    done = true;
                }
            }
        }
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *ps = TTF_RenderText_Blended(f, "Enter Host IP (e.g., 127.0.0.1):", white);
        if (ps)
        {
            SDL_Texture *pt = SDL_CreateTextureFromSurface(r, ps);
            SDL_Rect pr = {SCREEN_WIDTH / 2 - ps->w / 2, 200, ps->w, ps->h};
            SDL_RenderCopy(r, pt, NULL, &pr);
            SDL_FreeSurface(ps);
            SDL_DestroyTexture(pt);
        }
        SDL_Surface *is = TTF_RenderText_Blended(f, input[0] ? input : " ", white);
        if (is)
        {
            SDL_Texture *it = SDL_CreateTextureFromSurface(r, is);
            SDL_Rect ir = {SCREEN_WIDTH / 2 - is->w / 2, 250, is->w, is->h};
            SDL_RenderCopy(r, it, NULL, &ir);
            SDL_FreeSurface(is);
            SDL_DestroyTexture(it);
        }
        SDL_SetRenderDrawColor(r, 180, 0, 0, 255);
        SDL_RenderFillRect(r, &cb);
        SDL_Surface *cs = TTF_RenderText_Blended(f, "Confirm", white);
        if (cs)
        {
            SDL_Texture *ct = SDL_CreateTextureFromSurface(r, cs);
            SDL_Rect cr = {cb.x + (cb.w - cs->w) / 2, cb.y + (cb.h - cs->h) / 2, cs->w, cs->h};
            SDL_RenderCopy(r, ct, NULL, &cr);
            SDL_FreeSurface(cs);
            SDL_DestroyTexture(ct);
        }
        if (cursor > 0 && !is_valid_ip(input))
        {
            SDL_Surface *es = TTF_RenderText_Blended(f, "Invalid IP format", white);
            if (es)
            {
                SDL_Texture *et = SDL_CreateTextureFromSurface(r, es);
                SDL_Rect er = {SCREEN_WIDTH / 2 - es->w / 2, 300, es->w, es->h};
                SDL_RenderCopy(r, et, NULL, &er);
                SDL_FreeSurface(es);
                SDL_DestroyTexture(et);
            }
        }
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
    SDL_StopTextInput();
    TTF_CloseFont(f);
    return 0;
}

int showConnectionScreen(SDL_Renderer *r, UDPsocket s, IPaddress *peer, bool host)
{
    TTF_Font *f = TTF_OpenFont("shlop.ttf", 28);
    if (!f)
        return -1;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Event e;
    bool connected = false;
    Uint32 st = SDL_GetTicks(), lt = 0;
    UDPpacket *p = SDLNet_AllocPacket(512);
    if (!p)
    {
        TTF_CloseFont(f);
        return -1;
    }
    const char *err = NULL;
    while (!connected && (SDL_GetTicks() - st) < CONNECTION_TIMEOUT)
    {
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT)
            {
                SDLNet_FreePacket(p);
                TTF_CloseFont(f);
                return -1;
            }
        Uint32 now = SDL_GetTicks();
        if (host)
        {
            if (SDLNet_UDP_Recv(s, p))
            {
                Packet *pk = (Packet *)p->data;
                if (pk->type == PACKET_HANDSHAKE && pk->id == 1 && pk->handshake.handshake == 0)
                {
                    peer->host = p->address.host;
                    peer->port = p->address.port;
                    Packet ack = {PACKET_HANDSHAKE, 0, .handshake = {1}};
                    send_packet(s, *peer, &ack);
                    connected = true;
                }
            }
        }
        else
        {
            if (now - lt >= 50)
            {
                Packet h = {PACKET_HANDSHAKE, 1, .handshake = {0}};
                send_packet(s, *peer, &h);
                lt = now;
            }
            if (SDLNet_UDP_Recv(s, p))
            {
                Packet *pk = (Packet *)p->data;
                if (pk->type == PACKET_HANDSHAKE && pk->id == 0 && pk->handshake.handshake == 1)
                    connected = true;
            }
        }
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
        const char *msg = host ? "Waiting for client..." : "Connecting to host...";
        SDL_Surface *srf = TTF_RenderText_Blended(f, msg, white);
        if (srf)
        {
            SDL_Texture *t = SDL_CreateTextureFromSurface(r, srf);
            SDL_Rect tr = {SCREEN_WIDTH / 2 - srf->w / 2, SCREEN_HEIGHT / 2 - srf->h / 2, srf->w, srf->h};
            SDL_RenderCopy(r, t, NULL, &tr);
            SDL_FreeSurface(srf);
            SDL_DestroyTexture(t);
        }
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
    if (!connected)
    {
        err = !host && peer->host == 0 ? "Invalid host IP address" : "Connection timed out";
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
        SDL_Surface *es = TTF_RenderText_Blended(f, err, white);
        if (es)
        {
            SDL_Texture *et = SDL_CreateTextureFromSurface(r, es);
            SDL_Rect er = {SCREEN_WIDTH / 2 - es->w / 2, SCREEN_HEIGHT / 2 - es->h / 2, es->w, es->h};
            SDL_RenderCopy(r, et, NULL, &er);
            SDL_FreeSurface(es);
            SDL_DestroyTexture(et);
        }
        SDL_RenderPresent(r);
        SDL_Delay(2000);
    }
    SDLNet_FreePacket(p);
    TTF_CloseFont(f);
    return connected ? 0 : -1;
}

int showMenuScreen(SDL_Renderer *r, const char *title, const char *options[], int n)
{
    SDL_Event e;
    bool inMenu = true;
    int sel = 0;
    SDL_Rect btn[n];
    TTF_Font *f = TTF_OpenFont("shlop.ttf", 28), *tf = TTF_OpenFont("simbiot.ttf", 64);
    if (!f || !tf)
        return -1;
    for (int i = 0; i < n; i++)
    {
        btn[i] = (SDL_Rect){SCREEN_WIDTH / 2 - 100, 250 + i * 80, 200, 50};
    }
    while (inMenu)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                TTF_CloseFont(f);
                TTF_CloseFont(tf);
                return -1;
            }
            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_UP)
                    sel = (sel + n - 1) % n;
                if (e.key.keysym.sym == SDLK_DOWN)
                    sel = (sel + 1) % n;
                if (e.key.keysym.sym == SDLK_RETURN)
                {
                    inMenu = false;
                    TTF_CloseFont(f);
                    TTF_CloseFont(tf);
                    return sel;
                }
            }
            if (e.type == SDL_MOUSEMOTION)
            {
                int mx = e.motion.x, my = e.motion.y;
                for (int i = 0; i < n; i++)
                    if (mx >= btn[i].x && mx <= btn[i].x + btn[i].w && my >= btn[i].y && my <= btn[i].y + btn[i].h)
                        sel = i;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x, my = e.button.y;
                for (int i = 0; i < n; i++)
                    if (mx >= btn[i].x && mx <= btn[i].x + btn[i].w && my >= btn[i].y && my <= btn[i].y + btn[i].h)
                    {
                        inMenu = false;
                        TTF_CloseFont(f);
                        TTF_CloseFont(tf);
                        return i;
                    }
            }
        }
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
        SDL_Color red = {200, 0, 0, 255}, white = {255, 255, 255, 255};
        SDL_Surface *ts = TTF_RenderText_Blended(tf, title, red);
        if (ts)
        {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(r, ts);
            SDL_Rect tr = {SCREEN_WIDTH / 2 - ts->w / 2, 60, ts->w, ts->h};
            SDL_RenderCopy(r, tt, NULL, &tr);
            SDL_FreeSurface(ts);
            SDL_DestroyTexture(tt);
        }
        for (int i = 0; i < n; i++)
        {
            SDL_SetRenderDrawColor(r, i == sel ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(r, &btn[i]);
            SDL_Surface *s = TTF_RenderText_Blended(f, options[i], white);
            if (s)
            {
                SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
                SDL_Rect tr = {btn[i].x + (btn[i].w - s->w) / 2, btn[i].y + (btn[i].h - s->h) / 2, s->w, s->h};
                SDL_RenderCopy(r, t, NULL, &tr);
                SDL_FreeSurface(s);
                SDL_DestroyTexture(t);
            }
        }
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
    TTF_CloseFont(f);
    TTF_CloseFont(tf);
    return -1;
}

int showSettings(SDL_Renderer *r, SDL_Window *w)
{
    SDL_Event e;
    bool inSettings = true;
    TTF_Font *f = TTF_OpenFont("shlop.ttf", 28), *tf = TTF_OpenFont("simbiot.ttf", 48);
    if (!f || !tf)
        return -1;
    while (inSettings)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                return -1;
            if (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_RETURN))
                inSettings = false;
        }
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface *ts = TTF_RenderText_Blended(tf, "Settings", white);
        if (ts)
        {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(r, ts);
            SDL_Rect tr = {SCREEN_WIDTH / 2 - ts->w / 2, 50, ts->w, ts->h};
            SDL_RenderCopy(r, tt, NULL, &tr);
            SDL_FreeSurface(ts);
            SDL_DestroyTexture(tt);
        }
        SDL_Surface *vs = TTF_RenderText_Blended(f, "Volume", white);
        if (vs)
        {
            SDL_Texture *vt = SDL_CreateTextureFromSurface(r, vs);
            SDL_Rect vr = {100, 180, vs->w, vs->h};
            SDL_RenderCopy(r, vt, NULL, &vr);
            SDL_FreeSurface(vs);
            SDL_DestroyTexture(vt);
        }
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
    TTF_CloseFont(f);
    TTF_CloseFont(tf);
    return 0;
}

void handle_network(UDPsocket s, Player *lp, int *ls, Player *rp, Bullet b[], Mob m[], Powerup p[], int *rl, int *rs, float *rspeed, int *rdmg, ActiveEffects *le, ActiveEffects *re)
{
    UDPpacket *pkt = SDLNet_AllocPacket(512);
    if (!pkt)
        return;
    while (SDLNet_UDP_Recv(s, pkt))
    {
        Packet *pk = (Packet *)pkt->data;
        if (pk->id != local_player_id || pk->type == PACKET_GAME_STATE)
        {
            switch (pk->type)
            {
            case PACKET_PLAYER_POS:
                rp->rect.x = (int)pk->pos.x;
                rp->rect.y = (int)pk->pos.y;
                rp->angle = pk->pos.angle;
                break;
            case PACKET_BULLET:
                for (int i = 0; i < MAX_BULLETS; i++)
                    if (!b[i].active || (b[i].rect.x == (int)pk->bullet.x && b[i].rect.y == (int)pk->bullet.y))
                    {
                        b[i].rect.x = (int)pk->bullet.x;
                        b[i].rect.y = (int)pk->bullet.y;
                        b[i].rect.w = BULLET_SIZE;
                        b[i].rect.h = BULLET_SIZE;
                        b[i].dx = pk->bullet.dx;
                        b[i].dy = pk->bullet.dy;
                        b[i].active = pk->bullet.active;
                        b[i].owner = pk->id;
                        break;
                    }
                break;
            case PACKET_MOB_UPDATE:
                if (!is_host && pk->mob.index >= 0 && pk->mob.index < MAX_MOBS)
                {
                    m[pk->mob.index].rect.x = (int)pk->mob.x;
                    m[pk->mob.index].rect.y = (int)pk->mob.y;
                    m[pk->mob.index].rect.w = pk->mob.w;
                    m[pk->mob.index].rect.h = pk->mob.h;
                    m[pk->mob.index].active = pk->mob.active;
                    m[pk->mob.index].health = pk->mob.health;
                    m[pk->mob.index].type = pk->mob.type;
                }
                break;
            case PACKET_POWERUP_UPDATE:
                if (pk->powerup.index >= 0 && pk->powerup.index < MAX_POWERUPS)
                {
                    p[pk->powerup.index].active = pk->powerup.active;
                    p[pk->powerup.index].picked_up = pk->powerup.picked_up;
                    p[pk->powerup.index].type = pk->powerup.type;
                    p[pk->powerup.index].rect.x = (int)pk->powerup.x;
                    p[pk->powerup.index].rect.y = (int)pk->powerup.y;
                    p[pk->powerup.index].rect.w = 32;
                    p[pk->powerup.index].rect.h = 32;
                    p[pk->powerup.index].pickup_time = SDL_GetTicks();
                    if (p[pk->powerup.index].picked_up && !p[pk->powerup.index].sound_played)
                    {
                        switch (p[pk->powerup.index].type)
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
                        p[pk->powerup.index].sound_played = true;
                    }
                    if (is_host && pk->id == 1 && pk->powerup.picked_up)
                    {
                        apply_powerup_effect(&p[pk->powerup.index], rp, re, SDL_GetTicks());
                        Packet sp = {PACKET_GAME_STATE, 1, .state = {rp->lives, *rs, rp->speed, rp->damage}};
                        send_packet(s, pkt->address, &sp);
                        Packet pp = {PACKET_POWERUP_UPDATE, 0, .powerup = {pk->powerup.index, p[pk->powerup.index].active, p[pk->powerup.index].picked_up, p[pk->powerup.index].type, (float)p[pk->powerup.index].rect.x, (float)p[pk->powerup.index].rect.y}};
                        send_packet(s, pkt->address, &pp);
                    }
                }
                break;
            case PACKET_GAME_STATE:
                if (pk->id == local_player_id)
                {
                    lp->lives = pk->state.lives;
                    *ls = pk->state.score;
                    lp->speed = pk->state.speed;
                    lp->damage = pk->state.damage;
                }
                else
                {
                    rp->lives = pk->state.lives;
                    *rs = pk->state.score;
                    rp->speed = pk->state.speed;
                    rp->damage = pk->state.damage;
                }
                break;
            }
        }
    }
    SDLNet_FreePacket(pkt);
}

int main(int argc, char *argv[])
{
    UDPsocket udp_socket = NULL;
    IPaddress peer;
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || SDLNet_Init() < 0 || !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) || TTF_Init() == -1)
        return 1;

    SDL_Window *w = SDL_CreateWindow("Zombie Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
    if (!w || !r)
        return 1;

    init_sound();
    play_music("source/spelmusik.wav");

    int mr = skipMenu ? 0 : showMenuScreen(r, "Zombie Game", (const char *[]){"Single Player", "Multiplayer", "Settings"}, 3);
    if (mr == 0)
    {
        is_multiplayer = false;
        is_host = true;
        local_player_id = 0;
    }
    else if (mr == 1)
    {
        is_multiplayer = true;
        int mpr = showMenuScreen(r, "Multiplayer", (const char *[]){"Host Game", "Join Game", "Back"}, 3);
        if (mpr == 0)
        {
            is_host = true;
            local_player_id = 0;
            SDLNet_ResolveHost(&peer, NULL, UDP_PORT);
        }
        else if (mpr == 1)
        {
            is_host = false;
            local_player_id = 1;
            char peer_ip[16] = {0};
            if (showIPInputScreen(r, peer_ip, sizeof(peer_ip)) != 0 || !is_valid_ip(peer_ip) || SDLNet_ResolveHost(&peer, peer_ip, UDP_PORT) != 0)
            {
                SDL_DestroyRenderer(r);
                SDL_DestroyWindow(w);
                TTF_Quit();
                IMG_Quit();
                SDL_Quit();
                return 0;
            }
        }
        else
        {
            SDL_DestroyRenderer(r);
            SDL_DestroyWindow(w);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 0;
        }
        udp_socket = SDLNet_UDP_Open(is_host ? UDP_PORT : 0);
        if (!udp_socket || showConnectionScreen(r, udp_socket, &peer, is_host) != 0)
        {
            if (udp_socket)
                SDLNet_UDP_Close(udp_socket);
            SDL_DestroyRenderer(r);
            SDL_DestroyWindow(w);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }
    else if (mr == 2)
        showSettings(r, w);
    else
    {
        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    tex_player = IMG_LoadTexture(r, "resources/hitman1.png");
    tex_mob = IMG_LoadTexture(r, "resources/zombie1.png");
    tex_tiles = IMG_LoadTexture(r, "resources/tiles.png");
    tex_extralife = IMG_LoadTexture(r, "resources/extralife.png");
    tex_extraspeed = IMG_LoadTexture(r, "resources/extraspeed.png");
    tex_doubledamage = IMG_LoadTexture(r, "resources/doubledamage.png");
    tex_freezeenemies = IMG_LoadTexture(r, "resources/freezeenemies.png");
    if (!tex_player || !tex_mob || !tex_tiles || !tex_extralife || !tex_extraspeed || !tex_doubledamage || !tex_freezeenemies)
        return 1;

    srand((unsigned int)time(NULL));
    Player lp = create_player(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2, PLAYER_SIZE, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    Player rp = create_player(SCREEN_WIDTH / 2 + 50, SCREEN_HEIGHT / 2, PLAYER_SIZE, DEFAULT_PLAYER_SPEED, DEFAULT_PLAYER_DAMAGE, MAX_HEALTH);
    int ls = 0, rs = 0;
    float rspeed = DEFAULT_PLAYER_SPEED;
    int rdmg = DEFAULT_PLAYER_DAMAGE;
    Bullet b[MAX_BULLETS] = {0};
    Mob m[MAX_MOBS] = {0};
    Powerup p[MAX_POWERUPS] = {0};

    if (is_host)
    {
        for (int i = 0; i < MAX_MOBS; i++)
        {
            int x, y, type = rand() % 4, health = (type == 3) ? 2 : 1;
            do
            {
                x = rand() % (SCREEN_WIDTH - MOB_SIZE);
                y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
                m[i] = create_mob(x, y, MOB_SIZE, type, health);
            } while (check_mob_collision(&m[i].rect, m, i));
            if (is_multiplayer)
            {
                Packet mp = {PACKET_MOB_UPDATE, local_player_id, .mob = {i, (float)m[i].rect.x, (float)m[i].rect.y, m[i].rect.w, m[i].rect.h, m[i].active, m[i].health, m[i].type}};
                send_packet(udp_socket, peer, &mp);
            }
        }
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            p[i] = create_powerup(rand() % 4, rand() % (SCREEN_WIDTH - 32), rand() % (SCREEN_HEIGHT - 32));
            if (is_multiplayer)
            {
                Packet pp = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {i, p[i].active, p[i].picked_up, p[i].type, (float)p[i].rect.x, (float)p[i].rect.y}};
                send_packet(udp_socket, peer, &pp);
            }
        }
    }
    else
    {
        for (int i = 0; i < MAX_MOBS; i++)
            m[i].active = false;
        for (int i = 0; i < MAX_POWERUPS; i++)
            p[i].active = p[i].picked_up = false;
    }

    bool run = true;
    SDL_Event e;
    bool sp = false;
    ActiveEffects le = {0}, re = {0};
    Uint32 now, lmst = 0, lpst = 0;

    while (run)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                run = false;
            if ((e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDL_SCANCODE_SPACE && !sp))
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!b[i].active)
                    {
                        b[i].rect.x = lp.rect.x + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        b[i].rect.y = lp.rect.y + PLAYER_SIZE / 2 - BULLET_SIZE / 2;
                        b[i].rect.w = BULLET_SIZE;
                        b[i].rect.h = BULLET_SIZE;
                        b[i].active = true;
                        b[i].owner = local_player_id;
                        float dx = (float)mx - (lp.rect.x + PLAYER_SIZE / 2), dy = (float)my - (lp.rect.y + PLAYER_SIZE / 2);
                        float len = sqrtf(dx * dx + dy * dy);
                        b[i].dx = len > 0 ? BULLET_SPEED * (dx / len) : BULLET_SPEED;
                        b[i].dy = len > 0 ? BULLET_SPEED * (dy / len) : 0;
                        play_sound(SOUND_SHOOT);
                        if (is_multiplayer)
                        {
                            Packet bp = {PACKET_BULLET, local_player_id, .bullet = {(float)b[i].rect.x, (float)b[i].rect.y, b[i].dx, b[i].dy, b[i].active}};
                            send_packet(udp_socket, peer, &bp);
                        }
                        if (e.type == SDL_KEYDOWN)
                            sp = true;
                        break;
                    }
                }
            }
            if (e.type == SDL_KEYUP && e.key.keysym.sym == SDL_SCANCODE_SPACE)
                sp = false;
        }

        update_player(&lp, SDL_GetKeyboardState(NULL));
        if (is_multiplayer)
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            float dx = (float)mx - (lp.rect.x + PLAYER_SIZE / 2), dy = (float)my - (lp.rect.y + PLAYER_SIZE / 2);
            float angle = atan2f(dy, dx) * 180.0f / M_PI;
            Packet pp = {PACKET_PLAYER_POS, local_player_id, .pos = {(float)lp.rect.x, (float)lp.rect.y, angle}};
            send_packet(udp_socket, peer, &pp);
            handle_network(udp_socket, &lp, &ls, &rp, b, m, p, &rp.lives, &rs, &rspeed, &rdmg, &le, &re);
        }

        now = SDL_GetTicks();
        if (is_host)
        {
            for (int i = 0; i < MAX_MOBS; i++)
            {
                if (m[i].active)
                {
                    if (!le.freeze_active && !re.freeze_active)
                        update_mob(&m[i], lp.rect, rp.rect, is_multiplayer);
                    if (m[i].attacking && now - m[i].last_attack_time >= m[i].attack_interval)
                    {
                        float dl = sqrtf(powf(lp.rect.x + lp.rect.w / 2 - (m[i].rect.x + m[i].rect.w / 2), 2) + powf(lp.rect.y + lp.rect.h / 2 - (m[i].rect.y + m[i].rect.h / 2), 2));
                        float dr = sqrtf(powf(rp.rect.x + rp.rect.w / 2 - (m[i].rect.x + m[i].rect.w / 2), 2) + powf(rp.rect.y + rp.rect.h / 2 - (m[i].rect.y + m[i].rect.h / 2), 2));
                        if (is_multiplayer && dr < dl)
                            rp.lives -= (m[i].type == 3) ? 2 : 1;
                        else
                            lp.lives -= (m[i].type == 3) ? 2 : 1;
                        if (lp.lives < 0)
                            lp.lives = 0;
                        if (rp.lives < 0)
                            rp.lives = 0;
                        m[i].last_attack_time = now;
                        if (is_multiplayer)
                        {
                            Packet spl = {PACKET_GAME_STATE, 0, .state = {lp.lives, ls, lp.speed, lp.damage}};
                            Packet spr = {PACKET_GAME_STATE, 1, .state = {rp.lives, rs, rp.speed, rp.damage}};
                            send_packet(udp_socket, peer, &spl);
                            send_packet(udp_socket, peer, &spr);
                        }
                    }
                    if (is_multiplayer)
                    {
                        Packet mp = {PACKET_MOB_UPDATE, local_player_id, .mob = {i, (float)m[i].rect.x, (float)m[i].rect.y, m[i].rect.w, m[i].rect.h, m[i].active, m[i].health, m[i].type}};
                        send_packet(udp_socket, peer, &mp);
                    }
                }
            }
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (b[i].active)
                {
                    for (int j = 0; j < MAX_MOBS; j++)
                    {
                        if (m[j].active && SDL_HasIntersection(&b[i].rect, &m[j].rect))
                        {
                            m[j].health -= (b[i].owner == local_player_id) ? lp.damage : rp.damage;
                            if (m[j].health <= 0)
                            {
                                m[j].active = false;
                                if (b[i].owner == local_player_id)
                                    ls += 100;
                                else
                                    rs += 100;
                                if (rand() % 2 == 0)
                                {
                                    for (int k = 0; k < MAX_POWERUPS; k++)
                                    {
                                        if (!p[k].active && !p[k].picked_up)
                                        {
                                            p[k] = create_powerup(rand() % 4, m[j].rect.x, m[j].rect.y);
                                            if (is_multiplayer)
                                            {
                                                Packet pp = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {k, p[k].active, p[k].picked_up, p[k].type, (float)p[k].rect.x, (float)p[k].rect.y}};
                                                send_packet(udp_socket, peer, &pp);
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            b[i].active = false;
                            if (is_multiplayer)
                            {
                                Packet mp = {PACKET_MOB_UPDATE, local_player_id, .mob = {j, (float)m[j].rect.x, (float)m[j].rect.y, m[j].rect.w, m[j].rect.h, m[j].active, m[j].health, m[j].type}};
                                Packet bp = {PACKET_BULLET, local_player_id, .bullet = {(float)b[i].rect.x, (float)b[i].rect.y, b[i].dx, b[i].dy, b[i].active}};
                                send_packet(udp_socket, peer, &mp);
                                send_packet(udp_socket, peer, &bp);
                            }
                            break;
                        }
                    }
                }
            }
            if (now - lmst >= 1500)
            {
                for (int i = 0; i < MAX_MOBS; i++)
                {
                    if (!m[i].active)
                    {
                        int x, y, type = rand() % 4, health = (type == 3) ? 2 : 1;
                        do
                        {
                            x = rand() % (SCREEN_WIDTH - MOB_SIZE);
                            y = rand() % (SCREEN_HEIGHT - MOB_SIZE);
                            m[i] = create_mob(x, y, MOB_SIZE, type, health);
                        } while (check_mob_collision(&m[i].rect, m, i));
                        if (is_multiplayer)
                        {
                            Packet mp = {PACKET_MOB_UPDATE, local_player_id, .mob = {i, (float)m[i].rect.x, (float)m[i].rect.y, m[i].rect.w, m[i].rect.h, m[i].active, m[i].health, m[i].type}};
                            send_packet(udp_socket, peer, &mp);
                        }
                        break;
                    }
                }
                lmst = now;
            }
            if (now - lpst >= 5000)
            {
                for (int i = 0; i < MAX_POWERUPS; i++)
                {
                    if (!p[i].active || (p[i].picked_up && now - p[i].pickup_time >= p[i].duration))
                    {
                        p[i].active = p[i].picked_up = p[i].sound_played = false;
                        p[i] = create_powerup(rand() % 4, rand() % (SCREEN_WIDTH - 32), rand() % (SCREEN_HEIGHT - 32));
                        if (is_multiplayer)
                        {
                            Packet pp = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {i, p[i].active, p[i].picked_up, p[i].type, (float)p[i].rect.x, (float)p[i].rect.y}};
                            send_packet(udp_socket, peer, &pp);
                        }
                        break;
                    }
                }
                lpst = now;
            }
            if (is_multiplayer)
            {
                Packet spl = {PACKET_GAME_STATE, 0, .state = {lp.lives, ls, lp.speed, lp.damage}};
                Packet spr = {PACKET_GAME_STATE, 1, .state = {rp.lives, rs, rp.speed, rp.damage}};
                send_packet(udp_socket, peer, &spl);
                send_packet(udp_socket, peer, &spr);
            }
        }

        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            if (p[i].active && !p[i].picked_up && SDL_HasIntersection(&p[i].rect, &lp.rect))
            {
                if (is_host)
                {
                    p[i].picked_up = true;
                    p[i].pickup_time = now;
                    apply_powerup_effect(&p[i], &lp, &le, now);
                    Packet sp = {PACKET_GAME_STATE, local_player_id, .state = {lp.lives, ls, lp.speed, lp.damage}};
                    send_packet(udp_socket, peer, &sp);
                }
                else
                {
                    p[i].picked_up = true;
                    p[i].pickup_time = now;
                }
                if (!p[i].sound_played)
                {
                    switch (p[i].type)
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
                    p[i].sound_played = true;
                }
                if (is_multiplayer)
                {
                    Packet pp = {PACKET_POWERUP_UPDATE, local_player_id, .powerup = {i, p[i].active, p[i].picked_up, p[i].type, (float)p[i].rect.x, (float)p[i].rect.y}};
                    send_packet(udp_socket, peer, &pp);
                }
            }
        }

        update_effects(&le, &lp.speed, &lp.damage, now, p);
        if (is_multiplayer)
            update_effects(&re, &rp.speed, &rp.damage, now, p);

        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (b[i].active)
            {
                b[i].rect.x += (int)b[i].dx;
                b[i].rect.y += (int)b[i].dy;
                if (b[i].rect.x < 0 || b[i].rect.x > SCREEN_WIDTH || b[i].rect.y < 0 || b[i].rect.y > SCREEN_HEIGHT)
                {
                    b[i].active = false;
                    if (is_multiplayer && is_host)
                    {
                        Packet bp = {PACKET_BULLET, local_player_id, .bullet = {(float)b[i].rect.x, (float)b[i].rect.y, b[i].dx, b[i].dy, b[i].active}};
                        send_packet(udp_socket, peer, &bp);
                    }
                }
            }
        }

        if (!is_host)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (b[i].active && b[i].owner == local_player_id)
                {
                    for (int j = 0; j < MAX_MOBS; j++)
                    {
                        if (m[j].active && SDL_HasIntersection(&b[i].rect, &m[j].rect))
                        {
                            b[i].active = false;
                            break;
                        }
                    }
                }
            }
        }

        if (is_multiplayer)
        {
            rp.speed = rspeed;
            rp.damage = rdmg;
        }
        if (lp.lives <= 0 || (is_multiplayer && rp.lives <= 0))
        {
            int res = showMenuScreen(r, "GAME OVER", (const char *[]){"Play Again", "Main Menu", "Exit Game"}, 3);
            if (res == 0)
            {
                skipMenu = true;
                return main(argc, argv);
            }
            else if (res == 1)
            {
                skipMenu = false;
                return main(argc, argv);
            }
            else
                run = false;
        }

        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);
        int ts = 32;
        SDL_Rect src = {0, 0, ts, ts}, dst;
        for (int y = 0; y < SCREEN_HEIGHT; y += ts)
            for (int x = 0; x < SCREEN_WIDTH; x += ts)
            {
                dst = (SDL_Rect){x, y, ts, ts};
                SDL_RenderCopy(r, tex_tiles, &src, &dst);
            }

        draw_player(r, &lp);
        if (is_multiplayer)
            draw_player(r, &rp);
        draw_powerup_bars(r, &lp, p, now);
        if (is_multiplayer)
            draw_powerup_bars(r, &rp, p, now);
        for (int i = 0; i < MAX_MOBS; i++)
            if (m[i].active)
                draw_mob(r, &m[i], lp.rect);
        for (int i = 0; i < MAX_BULLETS; i++)
            if (b[i].active)
            {
                SDL_SetRenderDrawColor(r, b[i].owner == local_player_id ? 255 : 0, 0, 255, 255);
                SDL_RenderFillRect(r, &b[i].rect);
            }
        for (int i = 0; i < MAX_POWERUPS; i++)
            if (p[i].active && !p[i].picked_up)
                draw_powerup(r, &p[i]);

        SDL_Rect hb_bg = {10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
        SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
        SDL_RenderFillRect(r, &hb_bg);
        SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
        SDL_Rect hb = {10, 10, (HEALTH_BAR_WIDTH * lp.lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
        SDL_RenderFillRect(r, &hb);
        if (is_multiplayer)
        {
            SDL_Rect rhb_bg = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
            SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
            SDL_RenderFillRect(r, &rhb_bg);
            SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
            SDL_Rect rhb = {SCREEN_WIDTH - HEALTH_BAR_WIDTH - 10, 10, (HEALTH_BAR_WIDTH * rp.lives) / MAX_HEALTH, HEALTH_BAR_HEIGHT};
            SDL_RenderFillRect(r, &rhb);
        }

        SDL_RenderPresent(r);
        SDL_Delay(16);
    }

    if (is_multiplayer && udp_socket)
        SDLNet_UDP_Close(udp_socket);
    SDLNet_Quit();
    SDL_DestroyTexture(tex_player);
    SDL_DestroyTexture(tex_mob);
    SDL_DestroyTexture(tex_tiles);
    SDL_DestroyTexture(tex_extralife);
    SDL_DestroyTexture(tex_extraspeed);
    SDL_DestroyTexture(tex_doubledamage);
    SDL_DestroyTexture(tex_freezeenemies);
    cleanup_sound();
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}