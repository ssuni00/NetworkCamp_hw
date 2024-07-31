#include "server.h"

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *handle_client(void *arg)
{
    // arg로 받은 스레드 인수 구조체
    thread_args *args = (thread_args *)arg;
    int clnt_sd = args->clnt_sd;
    // 각 client 현재 작업 dir
    char *current_dir = args->current_dir;

    // dir info 전송 -> client
    send(clnt_sd, current_dir, strlen(current_dir) + 1, 0);

    Command command;

    while (1)
    {
        if (recv(clnt_sd, &command, sizeof(command), 0) <= 0)
        {
            close(clnt_sd);
            free(args);
            return NULL;
        }

        switch (command.command)
        {
        case cd:
            if (chdir(command.path) == 0)
            {
                // client별로 현재 dir update
                getcwd(current_dir, sizeof(args->current_dir));
            }
            else
            {
                strncpy(current_dir, "Failed to change directory", sizeof(args->current_dir) - 1);
                current_dir[sizeof(args->current_dir) - 1] = '\0';
            }
            // 결과 전송 -> client
            send(clnt_sd, current_dir, strlen(current_dir) + 1, 0);
            break;

        case ls:
        {
            char ls_command[BUFFER_SIZE];
            // 명령어 문자열 초기화 및 생성
            strncpy(ls_command, "ls ", sizeof(ls_command) - 1);
            ls_command[sizeof(ls_command) - 1] = '\0';

            strncat(ls_command, current_dir, sizeof(ls_command) - strlen(ls_command) - 1);
            strncat(ls_command, " ", sizeof(ls_command) - strlen(ls_command) - 1);
            strncat(ls_command, command.path, sizeof(ls_command) - strlen(ls_command) - 1);
            ls_command[sizeof(ls_command) - 1] = '\0';

            // 명령어 실행
            FILE *ls_output = popen(ls_command, "r");
            if (ls_output == NULL)
            {
                strncpy(ls_command, "Failed to execute ls command", sizeof(ls_command) - 1);
                ls_command[sizeof(ls_command) - 1] = '\0';
                send(clnt_sd, ls_command, strlen(ls_command) + 1, 0);
                break;
            }

            // 명령어 결과 읽고 전송
            char file_info[BUFFER_SIZE] = "";
            char line[BUFFER_SIZE];
            while (fgets(line, sizeof(line), ls_output) != NULL)
            {
                strncat(file_info, line, sizeof(file_info) - strlen(file_info) - 1);
            }

            pclose(ls_output);
            send(clnt_sd, file_info, strlen(file_info) + 1, 0);
            break;
        }

        case dl:
        {
            char file_path[BUFFER_SIZE];
            // current_dir와 command.path를 결합하여 파일 경로 생성
            strncpy(file_path, current_dir, sizeof(file_path) - 1);
            file_path[sizeof(file_path) - 1] = '\0';

            strncat(file_path, "/", sizeof(file_path) - strlen(file_path) - 1);
            strncat(file_path, command.path, sizeof(file_path) - strlen(file_path) - 1);
            file_path[sizeof(file_path) - 1] = '\0';

            int fd = open(file_path, O_RDONLY);
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

            char file_path[BUFFER_SIZE];
            // current_dir와 command.path를 결합하여 파일 경로 생성
            strncpy(file_path, current_dir, sizeof(file_path) - 1);
            file_path[sizeof(file_path) - 1] = '\0';

            strncat(file_path, "/", sizeof(file_path) - strlen(file_path) - 1);
            strncat(file_path, command.path, sizeof(file_path) - strlen(file_path) - 1);
            file_path[sizeof(file_path) - 1] = '\0';

            int fd = open(file_path, O_WRONLY | O_CREAT, 0666);
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
            free(args);
            return NULL;
        }
    }
}

int main(int argc, char *argv[])
{
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
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0)
        error_handling("bind() error");

    if (listen(serv_sd, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sd = accept(serv_sd, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        // 클라이언트 처리용 thread args 생성
        thread_args *args = malloc(sizeof(thread_args));
        args->clnt_sd = clnt_sd;
        // 클라이언트별 초기 디렉토리 설정
        getcwd(args->current_dir, BUFFER_SIZE);

        // 새로운 thread 생성 -> 클라이언트 요청 처리
        pthread_t thr;
        pthread_create(&thr, NULL, handle_client, args);
        pthread_detach(thr);
    }

    close(serv_sd);
    return 0;
}
