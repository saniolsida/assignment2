#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

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
    int serv_sock;
    int total_bytes = 0, file_bytes = 0;
    char message[BUF_SIZE];
    char file_name[FILE_SIZE];
    int str_len, receive = 0;
    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr, clnt_adr;
    packet_t packet;
    FILE *fp;
    clock_t start, end;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("UDP socket creation error");
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        total_bytes = 0;

        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0,
                           (struct sockaddr *)&clnt_adr, &clnt_adr_sz); // 파일이름과 전체 사이즈

        char *ptr = strtok(message, " ");
        strcpy(file_name, ptr);
        ptr = strtok(NULL, " ");
        file_bytes = atoi(ptr);

        printf("파일 이름과 사이즈: %s %d\n", file_name, file_bytes);
        fp = fopen(file_name, "wb");
        if (!fp)
            error_handling("fopen() failed");

        int i = 0;
        int past_seq = -1;

        while (file_bytes > 0)
        {
            // 구조체 받고 ack 전송
            str_len = recvfrom(serv_sock, (char *)&packet, sizeof(packet_t), 0,
                               (struct sockaddr *)&clnt_adr, &clnt_adr_sz); // 파일이름과 전체 사이즈

            printf("ACK %d\n", packet.seq);

            if (packet.seq == 3 && packet.seq != 0 && i < 5)
            {
                
            }
            else if(past_seq + 1 == packet.seq)
            {
                fwrite((void *)packet.buf, 1, packet.datalen, fp);

                char ack[10];
                str_len = snprintf(ack, 10, "%d", packet.seq);

                sendto(serv_sock, ack, str_len, 0,
                       (struct sockaddr *)&clnt_adr, clnt_adr_sz);
                past_seq = packet.seq;
                
                file_bytes -= packet.datalen;
            }

            i++;
        }

        fclose(fp);
    }

    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
