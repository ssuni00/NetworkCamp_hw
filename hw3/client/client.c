#include "client.h"

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

// command -> server & server -> respond
void execute_command(int socket, Command *command)
{
    send(socket, command, sizeof(*command), 0);
    char response[BUFFER_SIZE];
    recv(socket, response, sizeof(response), 0);
    printf("%s\n", response);
}

void download_file(int socket, const char *filename)
{
    Command command;
    command.command = DL;
    strncpy(command.path, filename, sizeof(command.path) - 1);
    command.path[sizeof(command.path) - 1] = '\0';
    send(socket, &command, sizeof(command), 0);

    ssize_t file_size;
    // 서버로부터 파일 크기 수신
    recv(socket, &file_size, sizeof(file_size), 0);

    int fd = open(filename, O_WRONLY | O_CREAT, 0666);

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

void upload_file(int socket, const char *filename, const char *server_path)
{
    Command command;
    command.command = UP;
    // 서버 경로 + 파일 이름 -> command.path에 저장
    snprintf(command.path, sizeof(command.path), "%s/%s", server_path, filename);
    send(socket, &command, sizeof(command), 0);

    int fd = open(filename, O_RDONLY);

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

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
    {
        error_handling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("connect() error");
    }

    // 서버의 현재 디렉토리 정보
    char current_dir[BUFFER_SIZE];
    // 서버로부터 현재 디렉토리 정보를 수신
    recv(sd, current_dir, sizeof(current_dir), 0);
    printf("server dir: %s\n", current_dir);

    // 사용자 명령어를 입력받을 버퍼
    char command_str[BUFFER_SIZE];

    while (1)
    {
        printf("> ");
        if (fgets(command_str, sizeof(command_str), stdin) == NULL)
        {
            break; // 입력이 없을 경우
        }

        // 입력된 명령어에서 개행 문자를 제거
        command_str[strcspn(command_str, "\n")] = '\0';

        Command command;

        // 여기도 서버처럼 swtich 쓰고싶었는데 cd, dl, up은 뒤에 공백때문에
        // 어차피 strncmp다 해줘야함 (if, else if로) 그럼 두번 일해야해서 그냥 if, elseif 사용
        if (strncmp(command_str, "cd ", 3) == 0)
        {
            command.command = CD;
            strncpy(command.path, command_str + 3, sizeof(command.path) - 1);
            command.path[sizeof(command.path) - 1] = '\0';
            execute_command(sd, &command);
        }
        else if (strncmp(command_str, "ls", 2) == 0)
        {
            command.command = LS;
            strncpy(command.path, command_str + 3, sizeof(command.path) - 1);
            command.path[sizeof(command.path) - 1] = '\0';
            execute_command(sd, &command);
        }
        else if (strncmp(command_str, "dl ", 3) == 0)
        {
            download_file(sd, command_str + 3);
        }
        else if (strncmp(command_str, "up ", 3) == 0)
        {
            // 파일 이름&서버 경로 추출
            char *filename = strtok(command_str + 3, " ");
            char *server_path = strtok(NULL, " ");
            if (filename && server_path)
            {
                upload_file(sd, filename, server_path);
            }
        }
        else if (strncmp(command_str, "exit", 4) == 0)
        {
            break;
        }
        else
        {
            printf("unknown command\n");
        }
    }

    close(sd);
    return 0;
}
