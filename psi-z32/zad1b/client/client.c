#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define DEFAULT_SRV_IP "172.21.32.10"
#define DEFAULT_PORT 5000
#define DATA_SIZE 5 * 1024 * 1024 // 5MB
#define BUF_SIZE 10 * 1024 // B

long long time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main(int argc, char *argv[])
{
    const char *host_addr = (argc > 1) ? argv[1] : DEFAULT_SRV_IP;
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;

    setvbuf(stdout, NULL, _IOLBF, 0);   /* logi widoczne od razu w docker logs */

    int sfd = 0;
    struct sockaddr_in server;

    printf("Start klienta...\n");

    sfd = socket( AF_INET, SOCK_STREAM, 0 );
    if (sfd == -1){
        perror("opening stream socket");
        exit(1);
    }

    server.sin_family = AF_INET;
    inet_aton( host_addr, &server.sin_addr );
    server.sin_port = htons(port);

    printf("Łączenie z %s:%d...\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    if (connect(sfd, (struct sockaddr *) &server, sizeof server) == -1){
        perror("connecting to server");
        exit(1);
    }
    printf("Połączono.\n");

    char *data = malloc(BUF_SIZE);
    memset(data, 'A', BUF_SIZE);
    long long before_time;
    long long after_time;
    long long time_diff;
    int written_data;
    long long total_written_data = 0;
    long long counter = 0;

    while(total_written_data < DATA_SIZE){
        // printf("%d:\tPróba wysłania danych...\n", counter);
        before_time = time_ms();
        if ((written_data = write(sfd, data, BUF_SIZE)) == -1){
            perror("writing data to stream");
            exit(1);
        }
        total_written_data += written_data;
        after_time = time_ms();
        time_diff = after_time - before_time;
        printf("%lld:\tWysłano %lldB danych. Łącznie: %lldB ", counter, written_data, total_written_data);
        printf("Czas wysyłania danych: %lldms\n", time_diff);

        if (time_diff > 0){
            printf("Opóźnienie %lldms podczas wysyłania bufora nr %lld! Wysłano łącznie %lldB danych\n", time_diff, counter, total_written_data);
        }
        counter++;
    }
    

    free(data);
    close(sfd);
    printf("Koniec wysyłania danych. Wysłano łącznie: %lldB\n", total_written_data);

    return 0;
}

