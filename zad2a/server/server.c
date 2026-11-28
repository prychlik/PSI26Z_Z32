#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 65535
#define DEFAULT_PORT 5000

int main(int argc, char *argv[]){
    int                      sfd, s;
    char                     buf[BUF_SIZE];
    ssize_t                  nread;
    socklen_t                peer_addrlen;
    struct sockaddr_in       server;
    struct sockaddr_storage  peer_addr;

    if ((sfd=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
        exit(1);
    }

    server.sin_family      = AF_INET;
    server.sin_port        = htons((argc > 1) ? atoi(argv[1]) : DEFAULT_PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if ((s=bind(sfd, (struct sockaddr *)&server, sizeof(server))) < 0){
        perror("bind()");
    }
    printf("bind() successful\n");


    printf("waiting for packets...\n");

    // for (;;) {
    //     long int counter = 0;
    //     char host[NI_MAXHOST], service[NI_MAXSERV];
    //     double stimestamp, rtimestamp;
    //     struct timeval tv;

    //     peer_addrlen = sizeof(peer_addr);
    //     nread = recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addrlen);
    //     printf("recvfrom ok\n");
    //     if (nread < 0) {
    //         fprintf(stderr, "failed recvfrom\n");
    //         continue;               /* Ignore failed request */
    //     }

    //     s = getnameinfo((struct sockaddr *) &peer_addr, peer_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
    //     if (s == 0)
    //         printf("Received %zd bytes from %s:%s\n", nread, host, service);
    //     else {
    //         fprintf(stderr, "getnameinfo() error: %s\n", gai_strerror(s));
    //     continue; 
    //     }

    //     sscanf( buf, "%ld %lf", &counter,  &stimestamp ); 
    //     gettimeofday(&tv, NULL);
    //     rtimestamp = timeinms(tv);
    //     // printf("r=%lf s=%lf ", rtimestamp, stimestamp);
    //     printf("Received counter = %ld time_delta = %10.2lf\n", counter, rtimestamp - stimestamp );
    // }
    close(sfd);
}
