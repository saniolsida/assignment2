#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

#define BUF_SIZE 256
#define FILE_SIZE 64
void error_handling(char *message);

typedef struct
{
    int seq;
    int datalen;
    char buf[BUF_SIZE];
} packet_t;

int main(int argc, char *argv[])
{
    int sock, i;
    char message[BUF_SIZE], file_name[FILE_SIZE], dub_file_name[BUF_SIZE], ACK[10] = "[ACK]", response[10];
    int str_len;
    int bytes;
    socklen_t adr_sz;
    struct stat st;
    struct sockaddr_in serv_adr, from_adr;
    struct timeval optVal = {1, 0};

    packet_t packet;

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

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &optVal, optLen);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    snprintf(file_name, FILE_SIZE, "%s", argv[3]);

    FILE *fp = fopen(file_name, "rb");
    if (!fp)
        error_handling("fopen() failed");

    printf("file name: %s\n", file_name);

    stat(file_name, &st);
    bytes = st.st_size; // 파일 size

    snprintf(dub_file_name, BUF_SIZE, "%s %d", file_name, bytes);

    sendto(sock, dub_file_name, strlen(dub_file_name), 0,
           (struct sockaddr *)&serv_adr, sizeof(serv_adr));

    adr_sz = sizeof(from_adr);

    i = 0;

    while (bytes > 0)
    {
        if (state > 0)
        {
            str_len = fread((void *)message, 1, BUF_SIZE, fp);
        }
        else
        {
            printf("파일 재전송 [SEQ]: %d\n", i);
        }

        state = 0;

        strcpy(packet.buf, message);
        packet.seq = i;
        packet.datalen = str_len;

        sendto(sock, (char *)&packet, sizeof(packet_t), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr)); // 파일 전송

        memset(&response,0,sizeof(response)); // 전송 후 초기화

        state = recvfrom(sock, response, 10, 0,
                         (struct sockaddr *)&from_adr, &adr_sz);

        if (state < 0)
        {
            printf("재전송 필요 %d\n", i);
        }
        else if (atoi(response) == packet.seq)
        {
            i++;
            printf("ACK: %s\n", response);
            bytes -= str_len;
        }
        else
        {
            state = 0;
        }
        memset(&response, 0, sizeof(response)); // 받고 초기화
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
