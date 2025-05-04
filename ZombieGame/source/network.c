#include "network.h"
#include <SDL2/SDL_log.h>
#include <string.h>
#include <stdbool.h>

IPaddress peerAddr;
UDPsocket netSocket = NULL;
UDPpacket *pktIn = NULL;
UDPpacket *pktOut = NULL;

#define RENDEZVOUS_HOST "34.2.48.101"
#define RENDEZVOUS_PORT 23456

bool network_rendezvous(NetMode mode, const char *roomID)
{
    IPaddress rvAddr;
    SDLNet_ResolveHost(&rvAddr, RENDEZVOUS_HOST, RENDEZVOUS_PORT);

    UDPpacket *rvPkt = SDLNet_AllocPacket(PACKET_SIZE);
    rvPkt->address = rvAddr;
    rvPkt->data[0] = (mode == MODE_HOST ? MSG_RV_REGISTER : MSG_RV_LOOKUP);
    strcpy((char *)rvPkt->data + 1, roomID);
    rvPkt->len = 1 + strlen(roomID) + 1;
    SDLNet_UDP_Send(netSocket, -1, rvPkt);

    // Vänta på peer‑addr – både host och join
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < 5000)
    {
        if (SDLNet_UDP_Recv(netSocket, rvPkt) && rvPkt->data[0] == MSG_RV_PEERADDR)
        {
            memcpy(&peerAddr, rvPkt->data + 1, sizeof(peerAddr));
            SDLNet_FreePacket(rvPkt);
            return true;
        }
    }
    SDLNet_FreePacket(rvPkt);
    return false;
}

int network_init(NetMode mode, const char *rendezvous_host, const char *roomID)

{
    if (SDLNet_Init() < 0)
    {
        SDL_Log("SDLNet_Init failed: %s", SDLNet_GetError());
        return -1;
    }

    // Öppna alltid en ephemeral port
    netSocket = SDLNet_UDP_Open(0);
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

    // Kör rendezvous – både HOST och JOIN får peerAddr satt här

    if (!network_rendezvous(mode, roomID))
    {
        SDL_Log("network_rendezvous failed");
        return -1;
    }

    // Hole‑punch mot peerAddr
    pktOut->len = 1;
    pktOut->data[0] = MSG_POS;
    for (int i = 0; i < 5; i++)
    {
        pktOut->address = peerAddr;
        SDLNet_UDP_Send(netSocket, -1, pktOut);
        SDL_Delay(50);
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

void network_send(int x, int y, float angle)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_POS;
    int off = 1;
    memcpy(d + off, &x, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &y, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &angle, sizeof(float));
    off += sizeof(float);
    pktOut->len = off;
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_spawn_mob(int idx, int x, int y, int type, int health)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_SPAWN_MOB;
    int off = 1;
    memcpy(d + off, &idx, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &x, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &y, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &type, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &health, sizeof(int));
    off += sizeof(int);
    pktOut->len = off;
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}
void network_send_spawn_powerup(int idx, int x, int y, int ptype)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_SPAWN_PWR;
    int off = 1;
    memcpy(d + off, &idx, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &x, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &y, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &ptype, sizeof(int));
    off += sizeof(int);
    pktOut->len = off;
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_remove_mob(int index)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_REMOVE_MOB;
    memcpy(d + 1, &index, sizeof(int));
    pktOut->len = 1 + sizeof(int);
    pktOut->address = peerAddr; // se till att peerAddr är satt
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_fire_bullet(int idx, int x, int y, float dx, float dy)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_FIRE_BULLET;
    int off = 1;

    memcpy(d + off, &idx, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &x, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &y, sizeof(int));
    off += sizeof(int);
    memcpy(d + off, &dx, sizeof(float));
    off += sizeof(float);
    memcpy(d + off, &dy, sizeof(float));
    off += sizeof(float);

    pktOut->len = off;
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_remove_powerup(int idx)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_REMOVE_PWR;
    memcpy(d + 1, &idx, sizeof(int));
    pktOut->len = 1 + sizeof(int);
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_remove_bullet(int idx)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_REMOVE_BULLET;
    memcpy(d + 1, &idx, sizeof(int));
    pktOut->len = 1 + sizeof(int);
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}
