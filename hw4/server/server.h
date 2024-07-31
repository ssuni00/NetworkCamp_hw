#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct
{
    char keyword[100];
    int count;
} SearchTerm;

SearchTerm terms[1000];
// 배열에 저장된 검색어의 총 개수를 추적하는 데 사용
int term_count = 0;

void start_server(int port);

#endif
