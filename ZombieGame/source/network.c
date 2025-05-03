#include "network.h"
#include <SDL2/SDL_log.h>
#include <string.h>
#include <stdbool.h>

static IPaddress peerAddr;

static UDPsocket netSocket = NULL;
static UDPpacket *pktIn = NULL;
static UDPpacket *pktOut = NULL;

int network_init(NetMode mode, const char *server_ip)
{
    if (SDLNet_Init() < 0)
    {
        SDL_Log("SDLNet_Init failed: %s", SDLNet_GetError());
        return -1;
    }

    int port = (mode == MODE_HOST) ? NET_PORT : 0;
    netSocket = SDLNet_UDP_Open(port);
    if (!netSocket)
    {
        SDL_Log("SDLNet_UDP_Open failed: %s", SDLNet_GetError());
        return -1;
    }

    pktIn = SDLNet_AllocPacket(PACKET_SIZE);
    pktOut = SDLNet_AllocPacket(PACKET_SIZE);
    if (!pktIn || !pktOut)
    {
        SDL_Log("SDLNet_AllocPacket failed");
        return -1;
    }

    if (mode == MODE_JOIN)
    {
        IPaddress addr;
        if (SDLNet_ResolveHost(&addr, server_ip, NET_PORT) < 0)
        {
            SDL_Log("SDLNet_ResolveHost failed: %s", SDLNet_GetError());
            return -1;
        }
        SDLNet_UDP_Bind(netSocket, -1, &addr);

        // --- Här sparar vi motpartens adress för framtida skickning ---
        peerAddr = addr;
    }

    return 0;
}

void network_shutdown(void)
{
    if (pktIn)
        SDLNet_FreePacket(pktIn);
    if (pktOut)
        SDLNet_FreePacket(pktOut);
    if (netSocket)
        SDLNet_UDP_Close(netSocket);
    SDLNet_Quit();
}

bool network_receive(int *out_x, int *out_y)
{
    if (SDLNet_UDP_Recv(netSocket, pktIn))
    {
        peerAddr = pktIn->address;

        memcpy(out_x, pktIn->data, sizeof(int));
        memcpy(out_y, pktIn->data + sizeof(int), sizeof(int));
        return true;
    }
    return false;
}

void network_send(int x, int y)
{
    pktOut->address = peerAddr;
    memcpy(pktOut->data, &x, sizeof(int));
    memcpy(pktOut->data + sizeof(int), &y, sizeof(int));
    pktOut->len = sizeof(int) * 2;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}
