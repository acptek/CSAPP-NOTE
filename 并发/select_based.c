// 多路复用非并发

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define PORT 4000
#define MAXCON 1024

void dosth() {
    puts("CIN START ..");
    sleep(10);
    puts("CIN END ..");
}

void doconn() {
    puts("SOCKET CONNECT ..");
    sleep(5);
    puts("CONNECT END ..");
}

int main(int argc, char **argv)
{
    int connfd;
    socklen_t clientlen;
    struct sockaddr_in laddr;
    fd_set read_set, ready_set;
    
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) {
        perror("socket()");
        exit(1);
    }

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    if(bind(sfd, (void *)&laddr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind()");
        exit(1);
    }

    if(listen(sfd, MAXCON) < 0) {
        perror("listen()");
        exit(1);
    }

    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    FD_SET(sfd, &read_set);

    while (1) {
        ready_set = read_set;
        select(sfd+1, &ready_set, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO, &ready_set))
            dosth();
        if(FD_ISSET(sfd, &ready_set)) {
            clientlen = sizeof(struct sockaddr_in);
            connfd = accept(sfd, (void*)&laddr, &clientlen);
            doconn();
            close(connfd);
        }
    }
    return 0;
}