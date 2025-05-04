#include "network.h"
#include <SDL2/SDL_log.h>
#include <string.h>
#include <stdbool.h>

IPaddress peerAddr;
UDPsocket netSocket = NULL;
UDPpacket *pktIn = NULL;
UDPpacket *pktOut = NULL;

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
    pktOut->address = peerAddr;
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

void network_send_freeze(void)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_FREEZE;
    pktOut->len = 1;
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_damage(int amount)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_DAMAGE;
    memcpy(d + 1, &amount, sizeof(int));
    pktOut->len = 1 + sizeof(int);
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_set_hp(int hp)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_SET_HP;
    int off = 1;
    memcpy(d + off, &hp, sizeof(int));
    pktOut->len = off + sizeof(int);
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}

void network_send_game_over(void)
{
    Uint8 *d = pktOut->data;
    d[0] = MSG_GAME_OVER;
    pktOut->len = 1;
    pktOut->address = peerAddr;
    SDLNet_UDP_Send(netSocket, -1, pktOut);
}