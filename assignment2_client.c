#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BUF_SIZE 256
#define FILE_SIZE 64
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock, i =0;
    char message[BUF_SIZE], file_name[FILE_SIZE], dub_file_name[FILE_SIZE], ACK[10] = "[ACK]", response[BUF_SIZE];
    int str_len;
    socklen_t adr_sz;
    struct sockaddr_in serv_adr, from_adr;
    struct timeval optVal = {1,0};
    int optLen = sizeof(optVal);

    int state = 1;

    if (argc != 4)
    {
        printf("Usage : %s <IP> <port> <file name>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO, &optVal,optLen);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    snprintf(file_name, FILE_SIZE, "%s", argv[3]);
    snprintf(dub_file_name, FILE_SIZE, "[FILE]%s[END]", argv[3]);

    printf("file name: %s\n", file_name);

    sendto(sock, dub_file_name, strlen(dub_file_name), 0,
           (struct sockaddr *)&serv_adr, sizeof(serv_adr));

    adr_sz = sizeof(from_adr);
    str_len = recvfrom(sock, message, BUF_SIZE, 0,
                       (struct sockaddr *)&from_adr, &adr_sz);

    message[str_len] = '\0';
    printf("Response from server: %s\n", message);

    FILE *fp = fopen(file_name, "rb");
    if (!fp)
        error_handling("fopen() failed");

    while (1)
    {
        if(state > 0){
            str_len = fread((void *)message, 1, BUF_SIZE, fp);
            message[str_len] = '\0';
        }else{
            printf("파일 재전송 [SEQ]: %d\n",i);
        }

        state = 0;

        sendto(sock, message, strlen(message), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr)); // 파일 전송

        snprintf(response, BUF_SIZE, "[PKT]%d[SEQ]", i);

        sendto(sock, response, strlen(response), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr)); // seq 전송
        
        memset(&response,0,sizeof(response)); // 전송 후 초기화

        state = recvfrom(sock, response, BUF_SIZE, 0,
                       (struct sockaddr *)&from_adr, &adr_sz);
        
        if(state < 0){
            printf("재전송 필요 %d\n",i);
        }else{
            i++;
            printf("%s\n",response);
        }
        memset(&response,0,sizeof(response)); // 받고 초기화

        if (str_len < BUF_SIZE && state > 0)
        {
            snprintf(message, BUF_SIZE, "[EOF]");
            printf("%s\n",message);
            sendto(sock, message, strlen(message), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr));

            break;
        }
    }
    fclose(fp);
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
