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

int main(int argc, char *argv[])
{
    int serv_sock;
    int total_bytes = 0;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr, clnt_adr;
    FILE *fp;
    clock_t start, end;
    // struct timeval timeout;
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
        while (1)
        {
            str_len = recvfrom(serv_sock, message, BUF_SIZE, 0,
                               (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
            message[str_len] = '\0';

            if (strncmp(message, "[FILE]", 6) == 0)
            {
                char file_name[FILE_SIZE];
                char *name = message + 6;
                char *end = strstr(name, "[END]");

                if (end)
                    *end = '\0';

                snprintf(file_name, sizeof(file_name), "%s", name);
                fp = fopen(file_name, "wb");
                if (!fp)
                    error_handling("fopen() failed");
                else
                    printf("file open: %s\n", file_name);

                sendto(serv_sock, file_name, str_len, 0,
                       (struct sockaddr *)&clnt_adr, clnt_adr_sz);

                memset(&message,0,sizeof(message));
                start = clock();
            }
            else if (fp && (strncmp(message, "[EOF]", 5) != 0) && (strncmp(message, "[PKT]", 5) != 0))
            {
                total_bytes += str_len;

                fwrite((void *)message, 1, str_len, fp);
            }
            else if(strncmp(message, "[EOF]", 5) == 0)
            {
                end = clock();
                printf("Thoughput: %.1lf\n",(total_bytes / ((double)(end - start)/CLOCKS_PER_SEC)));
                fclose(fp);
                break;
            }
            else if(strncmp(message, "[PKT]", 5) == 0)
            {
                printf("%s\n",message);
                char seq[FILE_SIZE];
                char *tmp_seq = message + 5;
                char *end = strstr(tmp_seq, "[SEQ]");

                if (end)
                    *end = '\0';

                snprintf(seq, sizeof(seq), "%s", tmp_seq);
                
                str_len = snprintf(message, BUF_SIZE, "[ACK] seq: %s",seq);
                // printf("%s\n",message);

                sendto(serv_sock, message, str_len, 0,
                       (struct sockaddr *)&clnt_adr, clnt_adr_sz);

                memset(&message,0,sizeof(message)); 
            }
        }
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
