#ifndef SERVER_H
#define SERVER_H

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
#include <pthread.h>
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

typedef struct
{
    int clnt_sd;
    char current_dir[BUFFER_SIZE];
} thread_args;

#endif
