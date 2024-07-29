#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>

#define BUF_SIZE 16384
#define TIMEOUT 3

typedef struct
{
    int seq_num;
    char data[BUF_SIZE];
} Packet;

typedef struct
{
    int ack_num;
} Ack;

void error_handling(char *message);

#endif
