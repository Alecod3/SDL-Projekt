#include "network.h"
#include <string.h>
#include <stdio.h>

NetMode net_mode = NET_NONE;
Player remote_player = {0};

static UDPsocket udpsock = NULL;
static IPaddress ip_remote;
static UDPpacket *packet = NULL;

int network_init(const char *host_ip)
{
    udpsock = SDLNet_UDP_Open(NET_PORT);
    if (!udpsock)
    {
        SDL_Log("UDP_Open: %s", SDLNet_GetError());
        return -1;
    }
    packet = SDLNet_AllocPacket(MAX_PACKET_LEN);
    if (!packet)
    {
        SDL_Log("AllocPacket: %s", SDLNet_GetError());
        return -1;
    }

    if (net_mode == NET_HOST)
    {
        // Host: vi tar emot på port NET_PORT, ingen bind behövs utan vi har redan öppnat
        SDL_Log("Hosting on port %d", NET_PORT);
    }
    else if (net_mode == NET_CLIENT)
    {
        // Klient: koppla upp mot host_ip på port NET_PORT
        if (SDLNet_ResolveHost(&ip_remote, host_ip, NET_PORT) < 0)
        {
            SDL_Log("ResolveHost: %s", SDLNet_GetError());
            return -1;
        }
        SDLNet_UDP_Bind(udpsock, 1, &ip_remote);
        SDL_Log("Client bound, will send to %s:%d", host_ip, NET_PORT);
    }
    return 0;
}

void network_send_state(const Player *local)
{
    if (net_mode == NET_NONE)
        return;
    // Paketstruktur: [x,y,angle]
    packet->len = snprintf((char *)packet->data, MAX_PACKET_LEN, "%d;%d\n",
                           local->rect.x, local->rect.y);
    packet->address = ip_remote;
    SDLNet_UDP_Send(udpsock, 1, packet);
}

void network_recv_state(void)
{
    if (net_mode == NET_NONE)
        return;
    if (SDLNet_UDP_Recv(udpsock, packet))
    {
        // Parsning
        int x, y;
        if (sscanf((char *)packet->data, "%d;%d", &x, &y) == 2)
        {
            remote_player.rect.x = x;
            remote_player.rect.y = y;
        }
    }
}

void network_cleanup(void)
{
    if (packet)
        SDLNet_FreePacket(packet);
    if (udpsock)
        SDLNet_UDP_Close(udpsock);
}
