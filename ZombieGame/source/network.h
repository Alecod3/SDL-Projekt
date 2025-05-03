#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include <stdbool.h>

#define NET_PORT 12345
#define PACKET_SIZE 32

typedef enum
{
    MODE_HOST,
    MODE_JOIN
} NetMode;

int network_init(NetMode mode, const char *server_ip);
void network_shutdown(void);
bool network_receive(int *out_x, int *out_y);
void network_send(int x, int y);

#endif
