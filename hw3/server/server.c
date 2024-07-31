#include "server.h"

// 에러 메시지를 출력하고 프로그램을 종료하는 함수
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

// 클라이언트 요청을 처리하는 함수, 각 클라이언트마다 스레드에서 실행됨
void *handle_client(void *arg)
{
    // arg로 받은 스레드 인수 구조체
    thread_args *args = (thread_args *)arg;
    int clnt_sd = args->clnt_sd;
    char *current_dir = args->current_dir; // 현재 작업 디렉토리

    // dir info 전송 -> client
    send(clnt_sd, current_dir, strlen(current_dir) + 1, 0);

    // client 명령수신 저장 구조체
    Command command;

    while (1)
    {
        // client 명령 수신
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
                // 현재 dir 업데이트
                getcwd(current_dir, sizeof(args->current_dir));
            }
            else
            {
                // dir 변경 실패
                strncpy(current_dir, "Failed to change directory", sizeof(args->current_dir) - 1);
                current_dir[sizeof(args->current_dir) - 1] = '\0';
            }
            // 결과 전송 -> client
            send(clnt_sd, current_dir, strlen(current_dir) + 1, 0);
            break;

        case ls:
        {
            char ls_command[BUFFER_SIZE];
            // ls 명령어 문자열을 구성 (버퍼 초과를 방지)
            int len = snprintf(ls_command, sizeof(ls_command), "ls %s/%s", current_dir, command.path);
            if (len >= sizeof(ls_command))
            {
                // 명령어가 너무 길 경우 오류 메시지 설정
                strncpy(ls_command, "Command too long", sizeof(ls_command) - 1);
                ls_command[sizeof(ls_command) - 1] = '\0';
            }

            // 명령어 실행
            FILE *ls_output = popen(ls_command, "r");
            if (ls_output == NULL)
            {
                // 명령어 실행 실패 시 오류 메시지 설정
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
            // 파일 경로 구성 (버퍼 초과를 방지)
            int len = snprintf(file_path, sizeof(file_path), "%s/%s", current_dir, command.path);
            if (len >= sizeof(file_path))
            {
                // 경로가 너무 길 경우 오류 코드를 전송
                ssize_t error = -1;
                send(clnt_sd, &error, sizeof(error), 0);
                break;
            }

            int fd = open(file_path, O_RDONLY);
            if (fd == -1)
            {
                // 파일 열기 실패 시 오류 코드를 전송
                ssize_t error = -1;
                send(clnt_sd, &error, sizeof(error), 0);
                break;
            }

            struct stat file_stat;
            // 파일의 상태 정보 받아옴
            fstat(fd, &file_stat);
            // 파일 크기 전송
            send(clnt_sd, &file_stat.st_size, sizeof(file_stat.st_size), 0);

            // 파일 데이터를 클라이언트로 전송
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
            // client -> 파일 크기 수신
            recv(clnt_sd, &file_size, sizeof(file_size), 0);

            char file_path[BUFFER_SIZE];
            // 파일 저장 경로 구성 (버퍼 초과를 방지)
            int len = snprintf(file_path, sizeof(file_path), "%s/%s", current_dir, command.path);
            if (len >= sizeof(file_path))
            {
                perror("File path too long");
                break;
            }

            int fd = open(file_path, O_WRONLY | O_CREAT, 0666);
            if (fd == -1)
            {
                perror("Failed to open file for writing");
                break;
            }

            // file data 수신&저장
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

        // client 처리용 thread arg생성
        thread_args *args = malloc(sizeof(thread_args));
        args->clnt_sd = clnt_sd;
        // 현재 디렉토리를 초기화
        getcwd(args->current_dir, BUFFER_SIZE);

        // 새로운 thread 생성 -> client 요청 처리
        pthread_t thr;
        pthread_create(&thr, NULL, handle_client, args);
        // thread 종료되면 리소스 자동 해제
        pthread_detach(thr);
    }

    close(serv_sd);
    return 0;
}
