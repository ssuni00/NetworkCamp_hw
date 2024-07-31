#include "client.h"

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

// 명령을 서버에 전송하고 응답을 출력
void execute_command(int socket, Command *command)
{
    send(socket, command, sizeof(*command), 0);
    char response[BUFFER_SIZE];
    recv(socket, response, sizeof(response), 0);
    printf("%s\n", response);
}

// 파일 다운로드
void download_file(int socket, const char *filename)
{
    Command command;
    command.command = dl;
    strncpy(command.path, filename, sizeof(command.path) - 1);
    command.path[sizeof(command.path) - 1] = '\0';
    send(socket, &command, sizeof(command), 0);

    ssize_t file_size;
    // 서버로부터 파일 크기 수신
    recv(socket, &file_size, sizeof(file_size), 0);

    if (file_size == -1)
    {
        printf("File not found on server.\n");
        return;
    }

    int fd = open(filename, O_WRONLY | O_CREAT, 0666);
    if (fd == -1)
    {
        perror("Failed to open file for writing");
        return;
    }

    char buffer[BUFFER_SIZE]; // 파일 내용을 받을 버퍼
    ssize_t bytes_received;

    // 파일 내용을 수신하여 파일에 write
    while (file_size > 0 && (bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0)
    {
        write(fd, buffer, bytes_received);
        file_size -= bytes_received; // 남은 파일 크기를 업데이트
    }
    close(fd);
    printf("Downloaded [%s]\n", filename);
}

// 파일 업로드
void upload_file(int socket, const char *filename, const char *server_path)
{
    Command command;
    command.command = up;
    // 서버 경로 + 파일 이름 -> command.path에 저장
    snprintf(command.path, sizeof(command.path), "%s/%s", server_path, filename);
    send(socket, &command, sizeof(command), 0);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open file for reading");
        return;
    }

    struct stat file_stat;
    // 파일 정보를 가져옴
    fstat(fd, &file_stat);
    send(socket, &file_stat.st_size, sizeof(file_stat.st_size), 0);

    // 파일 내용을 전송할 버퍼
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    // 파일 내용을 읽고 -> 서버로 전송
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        send(socket, buffer, bytes_read, 0); // 읽은 내용을 서버로 전송
    }
    close(fd);
    printf("Uploaded [%s]\n", filename);
}

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in serv_adr;

    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");

    // 서버의 현재 작업 디렉토리 수신 및 출력
    char current_dir[BUFFER_SIZE];
    recv(sd, current_dir, sizeof(current_dir), 0);
    printf("current dir: %s\n", current_dir);

    // 사용자 명령어를 입력받을 버퍼
    char command[BUFFER_SIZE];
    // 서버 경로 설정
    char server_path[BUFFER_SIZE] = ".";
    Command cmd;

    while (1)
    {
        printf("Enter command (cd/ls/dl/up)\n > ");
        fgets(command, sizeof(command), stdin);
        // 입력된 명령어에서 개행 문자를 제거
        command[strcspn(command, "\n")] = 0;

        if (strncmp(command, "cd", 2) == 0)
        {
            cmd.command = cd;
            strncpy(cmd.path, command + 3, sizeof(cmd.path) - 1);
            cmd.path[sizeof(cmd.path) - 1] = '\0';
            execute_command(sd, &cmd);
        }
        else if (strncmp(command, "ls", 2) == 0)
        {
            cmd.command = ls;
            strncpy(cmd.path, command + 3, sizeof(cmd.path) - 1);
            cmd.path[sizeof(cmd.path) - 1] = '\0';
            execute_command(sd, &cmd);
        }
        else if (strncmp(command, "dl", 2) == 0)
        {
            download_file(sd, command + 3);
        }
        else if (strncmp(command, "up", 2) == 0)
        {
            // 파일 이름과 서버 경로 추출
            char *local_file = strtok(command + 3, " ");
            char *remote_path = strtok(NULL, " ");
            if (remote_path == NULL)
            {
                remote_path = server_path;
            }
            upload_file(sd, local_file, remote_path);
        }
        else if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        else
        {
            printf("Unknown command: %s\n", command);
        }
    }

    close(sd);
    return 0;
}
