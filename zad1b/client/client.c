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
#define DATA_SIZE (5 * 1024 * 1024) // 5MB
#define DEFAULT_BUF_SIZE (10 * 1024) // B

long long time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int main(int argc, char *argv[])
{
    const char *host_addr = (argc > 1) ? argv[1] : DEFAULT_SRV_IP;
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
    size_t buf_size = (argc > 3) ? atoi(argv[3]) : DEFAULT_BUF_SIZE;

    setvbuf(stdout, NULL, _IOLBF, 0);   /* logi widoczne od razu w docker logs */

    int sfd = 0;
    struct sockaddr_in server;
    memset(&server, 0, sizeof server);

    printf("Start klienta...\n");

    sfd = socket( AF_INET, SOCK_STREAM, 0 );
    if (sfd == -1){
        perror("opening stream socket");
        exit(1);
    }

    server.sin_family = AF_INET;
    if(inet_aton( host_addr, &server.sin_addr ) == 0){
        perror("invalid ip address");
        exit(1);
    }
    server.sin_port = htons(port);

    printf("Łączenie z %s:%d...\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    if (connect(sfd, (struct sockaddr *) &server, sizeof server) == -1){
        perror("connecting to server");
        exit(1);
    }
    printf("Połączono.\n");

    char *data = malloc(buf_size);
    memset(data, 'A', buf_size);
    long long before_time;
    long long after_time;
    long long time_diff;
    int written_data;
    long long total_written_data = 0;
    long long counter = 0;
    long long delay_sum = 0;
    long long delay_count = 0;
    long long write_time = 0;
    long long max_delay = 0;
    long long start_time = 0;
    long long finish_time = 0;
    long long first_delay_bytes = -1;
    long long first_delay_time = -1;
    long long first_delay_value = 0;

    while(total_written_data < DATA_SIZE){
        size_t write_size = (DATA_SIZE - total_written_data < buf_size) ? (size_t)(DATA_SIZE - total_written_data) : buf_size; 
        before_time = time_us();
        if ((written_data = write(sfd, data, write_size)) == -1){
            perror("writing data to stream");
            exit(1);
        }
        after_time = time_us();
        total_written_data += written_data;
        time_diff = after_time - before_time;
        write_time += time_diff;
        // printf("%lld:\tWysłano %dB danych. Łącznie: %lldB Czas wysyłania danych: %lldus\n", counter, written_data, total_written_data, time_diff);

        if(counter == 0){
            start_time = before_time;
        }
        if(total_written_data >= DATA_SIZE){
            finish_time = after_time;
        }

        if (time_diff > 1000){
            delay_sum += time_diff;
            delay_count++;
            if (time_diff > max_delay){
                max_delay = time_diff;
            }
            if (first_delay_bytes == -1 && time_diff > 1000*1000){
                // za pierwsze faktyczne opóźnienie uznaję > 1s, bo zdarzają się takie np. 41ms
                first_delay_bytes = total_written_data - written_data;
                first_delay_time = before_time - start_time;
                first_delay_value = time_diff;
            }
            printf("Opóźnienie %lldus podczas wysyłania bufora nr %lld! Wysłano łącznie %lldB danych\n", time_diff, counter, total_written_data);
        }
        counter++;
    }
    

    free(data);
    close(sfd);
    printf("Koniec wysyłania danych.\n");
    printf("Rozmiar bufora: %zu\n", buf_size);
    printf("Wysłano łącznie: %lldB\n", total_written_data);
    printf("Całkowity czas transmisji: %.3fs\n", (finish_time - start_time) / 1000000.0);
    printf("Przepustowość: %.3f B/s\n", total_written_data / ((finish_time - start_time) / 1000000.0));
    printf("Liczba opóźnień: %lld\n", delay_count);
    printf("Największa wartość opóźnienia: %lldus\n", max_delay);
    printf("Pierwsze \"duże\" opóźnienie (%.3fms) po wysłaniu: %lldB, czas: %.3fs\n", first_delay_value/1000.0, first_delay_bytes, first_delay_time / 1000000.0);
    printf("Suma opóźnień wynosi: %lldus\n", delay_sum);
    printf("Łączny czas wykonywania funkcji write(): %lldus\n", write_time);


    return 0;
}

