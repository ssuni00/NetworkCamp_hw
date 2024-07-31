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

void error_handling(char *message);
void execute_command(int socket, Command *command);
void download_file(int socket, const char *filename);
void upload_file(int socket, const char *filename, const char *server_path);

#endif
