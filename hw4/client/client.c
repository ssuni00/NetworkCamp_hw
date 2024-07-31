#include "client.h"

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

// 대소문자 구분하지 않고 문자열 검색
char *strcasestr(const char *haystack, const char *needle)
{
    // 검색할 문자열이 비어있으면 전체 문자열 반환
    if (!*needle)
        return (char *)haystack;

    for (; *haystack; ++haystack)
    {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle))
        {
            const char *h, *n;
            for (h = haystack, n = needle; *h && *n; ++h, ++n)
            {
                if (tolower((unsigned char)*h) != tolower((unsigned char)*n))
                    break;
            }
            if (!*n) // needle 문자열의 끝에 도달했다면 일치하는 문자열을 찾은 것
                return (char *)haystack;
        }
    }
    // 일치하는 문자열이 없으면 NULL 반환
    return NULL;
}

void color_text(char *result, const char *query)
{
    int row = 2; // 검색 결과 출력 시작 위치 (2행부터)
    char lower_query[100];

    // 검색 쿼리를 소문자로 변환하여 lower_query에 저장
    for (int i = 0; query[i]; i++)
    {
        lower_query[i] = tolower((unsigned char)query[i]);
    }
    lower_query[strlen(query)] = '\0';

    if (strcmp(result, "No results found\n") == 0)
    {
        mvprintw(row, 0, "No results found");
        row++;
    }
    else
    {
        char *token = strtok(result, "\n");
        while (token != NULL)
        {
            char *pos = strcasestr(token, lower_query);
            if (pos)
            {
                int pre_match_len = pos - token;
                mvprintw(row, 0, "%.*s", pre_match_len, token);                               // 일치 부분 전까지 출력
                attron(A_BOLD | COLOR_PAIR(1));                                               // 강조 색상 켜기
                mvprintw(row, pre_match_len, "%.*s", (int)strlen(query), pos);                // 일치 부분 강조
                attroff(A_BOLD | COLOR_PAIR(1));                                              // 강조 색상 끄기
                mvprintw(row, pre_match_len + (int)strlen(query), "%s", pos + strlen(query)); // 나머지 출력
            }
            else
            {
                mvprintw(row, 0, "%s", token); // 일치하는 부분이 없으면 전체 출력
            }
            row++;
            token = strtok(NULL, "\n");
        }
    }
}

int main(int argc, char const *argv[])
{
    int sd;
    struct sockaddr_in serv_adr;
    char buffer[1024] = {0};
    char input[100] = {0};
    int input_length = 0;

    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");

    // ncurses 초기화
    initscr();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    mvprintw(0, 0, "Search Word: ");
    mvprintw(1, 0, "----------------");
    refresh();

    while (1)
    {
        // 사용자 입력 받기
        int ch = getch();
        if (ch == 27) // ESC 키를 누르면 종료
        {
            break;
        }
        else if (ch == 127 || ch == KEY_BACKSPACE || ch == 8) // 백스페이스 처리
        {
            if (input_length > 0)
            {
                // input buf에서 마지막 문자를 삭제
                // 문자열의 끝(\0) 설정
                input[--input_length] = '\0';
                // 입력된 문자열을 화면의 특정 위치에 다시 출력
                // 삭제된 문자를 반영
                // 20은 화면의 특정 위치를 가리킴
                mvprintw(0, 20, "%s   ", input);
                // 현재 커서 위치에서 줄의 끝까지의 화면 내용을 지움
                clrtoeol();
            }
        }
        else
        {
            input[input_length++] = (char)ch;
            input[input_length] = '\0';
        }

        // 사용자 입력 -> server 전송
        if (input_length > 0)
        {
            send(sd, input, strlen(input), 0);
        }
        else
        {
            // 입력이 없을 경우 공백 전송: no result 표시
            send(sd, " ", 1, 0);
        }

        // 서버로부터 결과 받기
        int valread = read(sd, buffer, 1024);
        buffer[valread] = '\0';

        clear(); // 이전 출력 지우기
        mvprintw(0, 0, "Search Word: %s", input);
        mvprintw(1, 0, "----------------\n");
        color_text(buffer, input);
        refresh();
    }

    endwin(); // ncurses 종료
    close(sd);

    return 0;
}
