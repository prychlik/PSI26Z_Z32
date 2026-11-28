#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEFAULT_PORT 5000
#define MAX_DGRAM    65535      /* największy możliwy datagram UDP/IPv4 */
#define HDR_SIZE     16         /* u32 seq + u64 ts + u32 size */
#define REPLY_SIZE   18         /* "OK" + u32 seq + u64 ts + u32 size */

static uint32_t get_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static uint64_t get_u64(const uint8_t *p)
{
    return ((uint64_t)get_u32(p) << 32) | get_u32(p + 4);
}

static void put_u64(uint8_t *p, uint64_t v)
{
    put_u32(p, (uint32_t)(v >> 32));
    put_u32(p + 4, (uint32_t)v);
}

int main(int argc, char *argv[])
{
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    setvbuf(stdout, NULL, _IOLBF, 0);   /* logi widoczne od razu w docker logs */

    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* większy bufor odbiorczy, żeby mieścił duże datagramy */
    int rcvbuf = 4 * 1024 * 1024;
    setsockopt(sfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 1;
    }

    uint8_t *buf = malloc(MAX_DGRAM);
    if (!buf) {
        perror("malloc");
        return 1;
    }

    printf("serwer UDP nasluchuje na porcie %d\n", port);

    for (;;) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);

        ssize_t n = recvfrom(sfd, buf, MAX_DGRAM, 0,
                             (struct sockaddr *)&cli, &len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        if (n < 16) {
            printf("za krotki datagram (%zdB) od %s\n", n, inet_ntoa(cli.sin_addr));
            continue;
        }

        uint32_t seq      = get_u32(buf);
        uint64_t ts       = get_u64(buf + 4);
        uint32_t declared = get_u32(buf + 12);

        /* weryfikacja: deklarowany rozmiar musi zgadzac sie z odebranym */
        if (declared != (uint32_t)n) {
            printf("seq %u: zly rozmiar (deklarowany %u, odebrany %zd)\n",
                   seq, declared, n);
            continue;
        }

        /* weryfikacja zawartosci: reszta datagramu to bajty 'A' */
        int payload_ok = 1;
        for (ssize_t i = 16; i < n; i++) {
            if (buf[i] != 'A') { payload_ok = 0; break; }
        }
        if (!payload_ok) {
            printf("seq %u: uszkodzona zawartosc\n", seq);
            continue;
        }

        uint8_t reply[REPLY_SIZE];
        reply[0] = 'O';
        reply[1] = 'K';
        put_u32(reply + 2, seq);
        put_u64(reply + 6, ts);
        put_u32(reply + 14, (uint32_t)n);

        if (sendto(sfd, reply, sizeof(reply), 0,
                   (struct sockaddr *)&cli, len) < 0)
            perror("sendto");
        else
            printf("seq %u: OK, %zdB od %s:%d\n", seq, n,
                   inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
    }
}
