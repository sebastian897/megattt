#include <string.h>
#include <stdbool.h>
// #include <stdio.h>
void StoreInputs(char* buf) 
{
    signed char id;
    bool thrust;
    bool shooting;
    signed char rotation;
    bool inputsPacket;

    memcpy(&inputsPacket, buf, sizeof(inputsPacket));
    if (inputsPacket){
        memcpy(&id, buf+sizeof(inputsPacket), sizeof(id));        
        memcpy(&thrust, buf+sizeof(inputsPacket)+sizeof(id), sizeof(thrust));
        memcpy(&rotation, buf+sizeof(inputsPacket)+sizeof(id)+sizeof(thrust), sizeof(rotation));
        memcpy(&shooting, buf+sizeof(inputsPacket)+sizeof(id)+sizeof(thrust)+sizeof(rotation), sizeof(shooting));
        // _inputs[id].rotation = rotation;
        // _inputs[id].thrust = thrust;
        // _inputs[id].shooting = shooting;
        // _players[id].active=true;
        // printf("Server: id = %d", id);
    } 
}
