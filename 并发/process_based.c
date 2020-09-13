#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define PORT 4000
#define MAXCON 1024
#define BUFSIZE 100

int sfd;

void sig_handler (int s) {
    puts("CLOSE BY SIGNAL");
    close(sfd);
    exit(0);
}

void dosth(struct sockaddr_in *raddr) {
    char buf[BUFSIZE];
    inet_ntop(AF_INET, &raddr->sin_addr, buf, sizeof(struct sockaddr_in));
    printf("Client - %s:%d\n", buf, ntohs(raddr->sin_port));
    printf("Start ..\n");
    sleep(10);
    printf("End ..\n");
}

// TCP-server
int main()
{
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaction(SIGINT, &sa, NULL);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) {
        perror("socket()");
        exit(1);
    }

    struct sockaddr_in laddr, raddr;
    laddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    laddr.sin_port = htons(PORT);
    bind(sfd, (void*)&laddr, sizeof(laddr));

    if(listen(sfd, MAXCON) < 0) {
        perror("listen()");
        exit(1);
    }

    while(1) {
        int clientlen = sizeof(struct sockaddr_in);
        int confd = accept(sfd, (void*)&raddr, &clientlen);
        if(confd < 0) {
            perror("accept()");
            exit(1);
        }

        int pid = fork();
        if(pid < 0) {
            perror("fork()");
            exit(1);
        }

        if(pid == 0) {
            close(sfd);
            dosth(&raddr);
            close(confd);
            exit(0);
        }
        close(confd);
    }

    exit(0);
}