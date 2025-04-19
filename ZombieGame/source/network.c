// network.c
#include "network.h"
#include <SDL2/SDL_net.h>
#include <stdio.h>
#include <string.h>

NetMode net_mode = NET_NONE;
Player remote_player = {0};

static UDPsocket udpsock = NULL;
static IPaddress ip_remote;
static UDPpacket *packet = NULL;

int network_init(const char *host_ip)
{
    // Initiera SDL_net
    if (SDLNet_Init() < 0)
    {
        SDL_Log("SDLNet_Init failed: %s", SDLNet_GetError());
        return -1;
    }

    // Host binder port 9999, klient binder ephemeral port 0
    int bind_port = (net_mode == NET_HOST ? NET_PORT : 0);
    udpsock = SDLNet_UDP_Open(bind_port);
    if (!udpsock)
    {
        SDL_Log("%s: UDP_Open(%d) failed: %s",
                (net_mode == NET_HOST ? "Host" : "Client"),
                bind_port, SDLNet_GetError());
        return -1;
    }

    packet = SDLNet_AllocPacket(MAX_PACKET_LEN);
    if (!packet)
    {
        SDL_Log("AllocPacket failed: %s", SDLNet_GetError());
        SDLNet_UDP_Close(udpsock);
        return -1;
    }

    // För klient: slå upp hostens adress och bind för sändning
    if (net_mode == NET_CLIENT)
    {
        if (SDLNet_ResolveHost(&ip_remote, host_ip, NET_PORT) < 0)
        {
            SDL_Log("ResolveHost(%s) failed: %s", host_ip, SDLNet_GetError());
            return -1;
        }
        SDLNet_UDP_Bind(udpsock, 1, &ip_remote);
    }

    return 0;
}

void network_send_state(const Player *local)
{
    if (net_mode == NET_NONE)
        return;
    // Formatera x;y
    packet->len = snprintf((char *)packet->data, MAX_PACKET_LEN, "%d;%d",
                           local->rect.x, local->rect.y);
    packet->address = ip_remote;
    SDLNet_UDP_Send(udpsock, 1, packet);
}

int network_recv_state(void)
{
    if (net_mode == NET_NONE)
        return 0;
    if (SDLNet_UDP_Recv(udpsock, packet))
    {
        int x, y;
        if (sscanf((char *)packet->data, "%d;%d", &x, &y) == 2)
        {
            remote_player.rect.x = x;
            remote_player.rect.y = y;
            return 1;
        }
    }
    return 0;
}

void network_cleanup(void)
{
    if (packet)
        SDLNet_FreePacket(packet);
    if (udpsock)
        SDLNet_UDP_Close(udpsock);
    SDLNet_Quit();
}
