// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include "player.h"

#define NET_PORT 9999
#define MAX_PACKET_LEN 256

typedef enum
{
    NET_NONE,
    NET_HOST,
    NET_CLIENT
} NetMode;

extern NetMode net_mode;
extern Player remote_player;

int network_init(const char *host_ip);
void network_send_state(const Player *local);
int network_recv_state(void);
void network_cleanup(void);

#endif // NETWORK_H
