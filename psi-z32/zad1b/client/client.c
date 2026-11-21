#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    const char *host = (argc > 1) ? argv[1] : "z32_zad1b_server"; // hostname
    int port = (argc > 2) ? atoi(argv[2]) : 5000;

    setvbuf(stdout, NULL, _IOLBF, 0);

    for (int i = 0; ; i++) {

        struct hostent *hp = gethostbyname(host);
        if (hp == NULL) {
            printf("%s: unknown host, ponawiam\n", host);
            sleep(2);
            continue;
        }

        struct sockaddr_in srv_addr = {0};
        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port = htons(port);
        memcpy(&srv_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

        int srv_sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (srv_sfd < 0) {
            perror("socket");
            return 1;
        }

        if (connect(srv_sfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) != 0) {
            perror("connect");
        } else {
            char msg[64], buf[64];
            snprintf(msg, sizeof(msg), "hello #%d", i);
            send(srv_sfd, msg, strlen(msg), 0);

            ssize_t n = recv(srv_sfd, buf, sizeof(buf) - 1, 0);
            if (n > 0) {
                buf[n] = '\0';
                printf("wyslano \"%s\", odpowiedz: %s\n", msg, buf);
            }
        }

        close(srv_sfd);
        sleep(2);
    }
}
