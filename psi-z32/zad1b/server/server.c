#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    int port = (argc > 1) ? atoi(argv[1]) : 5000;

    setvbuf(stdout, NULL, _IOLBF, 0);   /* logi widoczne od razu w docker logs */

    int srv_sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_sfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(srv_sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // every iface (eth0)
    addr.sin_port = htons(port);

    if (bind(srv_sfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) { perror("bind"); return 1; }
    if (listen(srv_sfd, 5) < 0) { perror("listen"); return 1; }

    printf("serwer nasluchuje na porcie %d\n", port);

    for (;;) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int cli_sfd = accept(srv_sfd, (struct sockaddr *)&cli, &len);
        if (cli_sfd < 0) {
            perror("accept");
            continue;
        }

        char buf[256];
        ssize_t n = recv(cli_sfd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            printf("od %s: %s\n", inet_ntoa(cli.sin_addr), buf);
            sleep(2);
            send(cli_sfd, "OK", 2, 0);
        }
        close(cli_sfd);
    }
}
