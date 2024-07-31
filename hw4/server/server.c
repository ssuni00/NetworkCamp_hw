#include "server.h"

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

// data.txt에서 검색어와 검색 횟수 load
void load_data()
{
    FILE *file = fopen("data.txt", "r");
    if (!file)
    {
        error_handling("Failed to open the file");
        exit(1);
    }
    // data.txt에서 검색어와 검색 횟수 읽고 -> terms 배열에 저장
    while (fscanf(file, "%99[^0-9] %d", terms[term_count].keyword, &terms[term_count].count) == 2)
    {
        // 검색어에서 끝에 있는 공백 제거
        char *end = terms[term_count].keyword + strlen(terms[term_count].keyword);
        // ptr가 문자열의 시작보다 뒤에 있는지 확인 (=검색어 문자열의 시작을 넘지 않는지 확인)
        // end ptr가 가리키는 위치 이전의 문자가 공백 문자 인지 검사
        while (end > terms[term_count].keyword && isspace((unsigned char)*(end - 1)))
        // isspace가 정의하는 공백 문자: space, tap, enter, ...
        {
            // 공백 문자가 발견되면 end 포인터를 한 문자 앞으로 이동
            // 문자열 끝에서 시작하여 공백이 아닌 문자를 만날 때까지 반복
            --end;
        }
        *end = '\0';
        term_count++;
    }
    fclose(file);
}

// 검색어를 검색 횟수에 따라 내림차순으로 정렬
int compare_terms(const void *a, const void *b)
{
    SearchTerm *termA = (SearchTerm *)a;
    SearchTerm *termB = (SearchTerm *)b;
    return termB->count - termA->count;
}

// client의 검색어에 따라 상위 검색어를 찾음
void find_top_terms(const char *word, char *result)
{
    int count = 0;
    SearchTerm filtered_terms[1000];
    int filtered_count = 0;
    char lower_word[BUFFER_SIZE];

    // 입력된 검색어가 비어있거나 공백만 있을 경우
    if (strlen(word) == 0 || (strlen(word) == 1 && isspace((unsigned char)word[0])))
    {
        strcpy(result, "No results found\n");
        return;
    }

    // 검색어를 소문자로 변환하여 lower_word에 저장
    for (int i = 0; word[i]; i++)
    {
        lower_word[i] = tolower((unsigned char)word[i]);
    }
    lower_word[strlen(word)] = '\0';

    // data.txt에서 소문자로 변환된 검색어를 사용하여 일치하는 항목 찾기
    for (int i = 0; i < term_count; i++)
    {
        char lower_keyword[100];
        for (int j = 0; terms[i].keyword[j]; j++)
        {
            lower_keyword[j] = tolower((unsigned char)terms[i].keyword[j]);
        }
        lower_keyword[strlen(terms[i].keyword)] = '\0';

        // 검색어가 포함된 항목을 filtered_terms에 저장
        if (strstr(lower_keyword, lower_word) != NULL)
        {
            filtered_terms[filtered_count++] = terms[i];
        }
    }

    // 필터링된 항목이 없을 경우
    if (filtered_count == 0)
    {
        strcpy(result, "No results found\n");
    }
    else
    {
        // 검색 횟수에 따라 필터링된 항목을 정렬
        qsort(filtered_terms, filtered_count, sizeof(SearchTerm), compare_terms);

        // 상위 10개 검색어를 결과 문자열에 추가
        for (int i = 0; i < filtered_count && count < 10; i++)
        {
            sprintf(result + strlen(result), "%s\n", filtered_terms[i].keyword);
            count++;
        }
    }
}

void *handle_client(void *arg)
{
    int clnt_sock = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1)
    {
        // client data 수신
        int bytes_read = recv(clnt_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0)
            break;

        buffer[bytes_read] = '\0';
        // 초기화
        response[0] = '\0';
        find_top_terms(buffer, response);
        send(clnt_sock, response, strlen(response), 0);
    }
    close(clnt_sock);
    return NULL;
}

int main(int argc, char **argv)
{

    int serv_sd, clnt_sd;
    // struct sockaddr_in adr;
    struct sockaddr_in serv_adr;
    // int opt = 1;
    int addrlen = sizeof(serv_adr);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    load_data();

    serv_sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0)
        error_handling("bind() error");

    if (listen(serv_sd, 5) == -1)
        error_handling("listen() error");

    // 클라이언트 연결 수락 및 처리
    while (1)
    {
        clnt_sd = accept(serv_sd, (struct sockaddr *)&serv_adr, (socklen_t *)&addrlen);
        int *new_clnt_sd = malloc(sizeof(int));
        // clnt_sd -> thread <->client소통에 사용
        *new_clnt_sd = clnt_sd;
        // thread 식별자를 저장
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, new_clnt_sd);
    }

    return 0;
}
