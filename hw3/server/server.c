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
    // 현재 작업 디렉토리 저장할 버퍼
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
            // ls 명령어 실행 & 출력 스트림 열기
            FILE *ls_output = popen(ls_command, "r");
            if (ls_output == NULL)
            {
                // ls 명령어 실행 실패 시 에러 메시지 전송
                strncpy(ls_command, "Failed to execute ls command", sizeof(ls_command) - 1);
                ls_command[sizeof(ls_command) - 1] = '\0';
                send(clnt_sd, ls_command, strlen(ls_command) + 1, 0); // 에러 메시지를 클라이언트로 전송
                break;
            }

            char file_info[BUFFER_SIZE] = "";
            // ls 결과물 한 줄씩 읽어올 버퍼
            char line[BUFFER_SIZE];

            // ls 명령어 출력 결과를 한 줄씩 읽어옴
            // ls -al, ls -a, 등 여기서 ls 뒤에 것도 가져옴
            while (fgets(line, sizeof(line), ls_output) != NULL)
            {
                // 각 줄을 file_info 버퍼에 추가
                strncat(file_info, line, sizeof(file_info) - strlen(file_info) - 1);
            }

            pclose(ls_output); // 명령어 실행 후 파일 스트림 닫기
            // 결합된 ls 출력 결과를 클라이언트에게 전송
            send(clnt_sd, file_info, strlen(file_info) + 1, 0);
            break;
        }

        case dl:
        {
            int fd = open(command.path, O_RDONLY);

            // 파일 정보 저장
            struct stat file_stat;
            // 파일의 상태 정보
            fstat(fd, &file_stat);
            // 파일 크기 -> client 전송
            send(clnt_sd, &file_stat.st_size, sizeof(file_stat.st_size), 0);

            // 파일 내용을 읽어올 버퍼
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read;
            // 파일 읽고 -> client 전송
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

            // 파일 크기를 클라이언트로부터 수신
            recv(clnt_sd, &file_size, sizeof(file_size), 0);
            int fd = open(command.path, O_WRONLY | O_CREAT, 0666);

            // 파일 내용을 수신할 버퍼
            char buffer[BUFFER_SIZE];
            ssize_t bytes_received;
            // 파일 내용 수신 -> 파일에 write
            while (file_size > 0 && (bytes_received = recv(clnt_sd, buffer, sizeof(buffer), 0)) > 0)
            {
                write(fd, buffer, bytes_received);
                // 남은 파일 크기 update
                file_size -= bytes_received;
            }
            close(fd);
            break;
        }
        default:
            // 알 수 없는 명령 수신 -> 에러 전송 & 클라이언트 소켓 닫기
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

    // 서버 주소 구조체 초기화
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(port);

    // 소켓 바인딩
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
