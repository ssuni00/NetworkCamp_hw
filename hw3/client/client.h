#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFFER_SIZE 1024

typedef enum
{
    cd,
    ls,
    dl,
    up
} CommandType;

typedef struct
{
    CommandType command;
    char path[BUFFER_SIZE];
} Command;

#endif
