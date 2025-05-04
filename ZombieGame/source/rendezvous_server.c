// rendezvous_server.c
#include <SDL2/SDL_net.h>
#include <string.h>
#include <stdio.h>

#define PACKET_SIZE 32
#define RENDEZVOUS_PORT 23456

typedef enum
{
    MSG_RV_REGISTER = 100,
    MSG_RV_LOOKUP = 101,
    MSG_RV_PEERADDR = 102
} MsgType;

typedef struct
{
    char roomID[24];
    IPaddress addr;
} Entry;

int main(int argc, char **argv)
{
    SDLNet_Init();
    UDPsocket sock = SDLNet_UDP_Open(RENDEZVOUS_PORT);
    UDPpacket *pkt = SDLNet_AllocPacket(PACKET_SIZE);
    Entry entries[64] = {0};
    int entryCount = 0;

    while (1)
    {
        if (SDLNet_UDP_Recv(sock, pkt))
        {
            char *room = (char *)pkt->data + 1;
            if (pkt->data[0] == MSG_RV_REGISTER)
            {
                // spara host
                strcpy(entries[entryCount].roomID, room);
                entries[entryCount].addr = pkt->address;
                entryCount++;
            }
            else if (pkt->data[0] == MSG_RV_LOOKUP)
            {
                // hitta host för room
                for (int i = 0; i < entryCount; i++)
                {
                    if (strcmp(entries[i].roomID, room) == 0)
                    {
                        // skicka tillbaka host‑addr
                        pkt->data[0] = MSG_RV_PEERADDR;
                        memcpy(pkt->data + 1, &entries[i].addr, sizeof(IPaddress));
                        pkt->len = 1 + sizeof(IPaddress);
                        SDLNet_UDP_Send(sock, -1, pkt);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}
