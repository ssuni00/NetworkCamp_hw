#include "server.h"

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *handle_client(void *clnt_sd_ptr)
{
    // client sd를 포인터에서 역참조하여 가져옴
    int clnt_sd = *((int *)clnt_sd_ptr);
    free(clnt_sd_ptr);

    Command command;
    char current_dir[BUFFER_SIZE];

    // 현재 작업 디렉토리 -> client 전송
    getcwd(current_dir, sizeof(current_dir));
    send(clnt_sd, current_dir, strlen(current_dir) + 1, 0);

    while (1)
    {
        // client 명령 수신 실패
        if (recv(clnt_sd, &command, sizeof(command), 0) <= 0)
        {
            close(clnt_sd);
            return NULL; // thread 종료
        }

        switch (command.command)
        {
        case cd:
            // 디렉토리 변경 성공
            if (chdir(command.path) == 0)
            {
                // 현재 디렉토리 업데이트
                getcwd(current_dir, sizeof(current_dir));
            }
            else
            {
                strncpy(current_dir, "Failed to change directory", sizeof(current_dir) - 1);
                current_dir[sizeof(current_dir) - 1] = '\0';
            }
            // 업데이트된 현재 디렉토리 or 에러 메시지 -> client 전송
            send(clnt_sd, current_dir, strlen(current_dir) + 1, 0); // 결과를 클라이언트로 전송
            break;

        case ls:
        {
            char ls_command[BUFFER_SIZE];
            snprintf(ls_command, sizeof(ls_command), "ls %.*s", (int)(sizeof(ls_command) - 4 - 1), command.path);
            FILE *ls_output = popen(ls_command, "r");
            if (ls_output == NULL)
            {
                strncpy(ls_command, "Failed to execute ls command", sizeof(ls_command) - 1);
                ls_command[sizeof(ls_command) - 1] = '\0';
                send(clnt_sd, ls_command, strlen(ls_command) + 1, 0); // 에러 메시지를 클라이언트로 전송
                break;
            }

            char file_info[BUFFER_SIZE] = "";
            char line[BUFFER_SIZE];

            while (fgets(line, sizeof(line), ls_output) != NULL)
            {
                strncat(file_info, line, sizeof(file_info) - strlen(file_info) - 1);
            }

            pclose(ls_output); // 명령어 실행 후 파일 스트림 닫기
            send(clnt_sd, file_info, strlen(file_info) + 1, 0);
            break;
        }

        case dl:
        {
            int fd = open(command.path, O_RDONLY);
            if (fd == -1)
            {
                ssize_t error = -1;
                send(clnt_sd, &error, sizeof(error), 0);
                break;
            }

            struct stat file_stat;
            fstat(fd, &file_stat);
            send(clnt_sd, &file_stat.st_size, sizeof(file_stat.st_size), 0);

            char buffer[BUFFER_SIZE];
            ssize_t bytes_read;
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
            {
                send(clnt_sd, buffer, bytes_read, 0);
            }
            close(fd);
            break;
        }

        case up:
        {
            ssize_t file_size;
            recv(clnt_sd, &file_size, sizeof(file_size), 0);

            int fd = open(command.path, O_WRONLY | O_CREAT, 0666);
            if (fd == -1)
            {
                perror("Failed to open file for writing");
                break;
            }

            char buffer[BUFFER_SIZE];
            ssize_t bytes_received;
            while (file_size > 0 && (bytes_received = recv(clnt_sd, buffer, sizeof(buffer), 0)) > 0)
            {
                write(fd, buffer, bytes_received);
                file_size -= bytes_received;
            }
            close(fd);
            break;
        }
        default:
            error_handling("unknown command");
            close(clnt_sd);
            return NULL;
        }
    }
}

int main(int argc, char *argv[])
{
    int port = atoi(argv[1]);
    int serv_sd, clnt_sd;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sd == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(port);

    if (bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0)
    {
        error_handling("bind() error");
    }

    if (listen(serv_sd, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sd = accept(serv_sd, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (clnt_sd < 0)
        {
            error_handling("accept() error!");
        }

        int *clnt_sd_ptr = malloc(sizeof(int));
        *clnt_sd_ptr = clnt_sd;

        pthread_t thr;
        pthread_create(&thr, NULL, handle_client, clnt_sd_ptr);
        pthread_detach(thr);
    }

    close(serv_sd);
    return 0;
}
