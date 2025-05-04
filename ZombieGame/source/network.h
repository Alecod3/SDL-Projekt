#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include <stdbool.h>

extern IPaddress peerAddr;
extern UDPsocket netSocket;
extern UDPpacket *pktIn;
extern UDPpacket *pktOut;

#define NET_PORT 12345
#define PACKET_SIZE 32

typedef enum
{
    MODE_HOST,
    MODE_JOIN
} NetMode;

typedef enum
{
    MSG_POS = 0,
    MSG_SPAWN_MOB = 1,
    MSG_SPAWN_PWR = 2,
    MSG_REMOVE_MOB = 3,
    MSG_FIRE_BULLET = 4,
    MSG_REMOVE_PWR = 5,
    MSG_REMOVE_BULLET = 6,
    MSG_FREEZE = 7,
    MSG_DAMAGE = 8,
    MSG_SET_HP = 9,
    MSG_GAME_OVER = 10
} MsgType;

void network_send_set_hp(int hp);
void network_send_spawn_mob(int idx, int x, int y, int type, int health);
void network_send_spawn_powerup(int idx, int x, int y, int powerupType);
void network_send_remove_mob(int index);
void network_send_fire_bullet(int idx, int x, int y, float dx, float dy);
void network_send_remove_powerup(int idx);
void network_send_remove_bullet(int idx);
void network_send_freeze(void);
void network_send_damage(int amount);
void network_send_game_over(void);
int network_init(NetMode mode, const char *server_ip);
void network_shutdown(void);
bool network_receive(int *out_x, int *out_y);
void network_send(int x, int y, float angle);

#endif