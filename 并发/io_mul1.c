#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#define RIO_BUFSIZE 4096
#define MAXCON 1024
#define MAXLINE 1024
#define MAXCLIENT 1024
#define PORT 4000

char buf[MAXLINE];

struct Stat {
    fd_set rset, allset;
    int client[MAXCLIENT];
    int maxfd;
    int nready;
    int maxi;
};

void Stat_init(struct Stat *p) {
    FD_ZERO(&p->allset);
    p->maxfd = -1;
    p->nready = 0;
    p->maxi = 0;
    for(int i = 0; i < MAXCLIENT; ++i) {
        p->client[i] = -1;
    }
}

int main()
{
    struct Stat stat;
    Stat_init(&stat);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) {
        perror("socket()");
        exit(1);
    }

    puts("hfjkhsdkfhskfhd");
    struct sockaddr_in laddr;
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    bind(sfd, (void*)&laddr, sizeof(struct sockaddr_in));

    listen(sfd, MAXCON);

    stat.maxfd = sfd;
    FD_SET(sfd, &stat.allset);
    // sfd 只是监听描述符，不需要加入到client中

    while(1) {
        stat.rset = stat.allset;
        stat.nready = select(stat.maxfd+1, &stat.rset, NULL, NULL, NULL);

        // 新的连接到来事件
        if(FD_ISSET(sfd, &stat.rset)) {
            struct sockaddr_in raddr;
            socklen_t clientlen = sizeof(raddr);
            int connfd = accept(sfd, (void*)&raddr, &clientlen);
            // 加入client
            int i;
            for(i = 0; i < MAXCON; ++i) {
                if(stat.client[i] < 0) {
                    stat.client[i] = connfd;
                    break;
                }
            }
            if(i == MAXCON) {
                perror("No Client Space");
                exit(1);
            }
            FD_SET(connfd, &stat.allset);
            if(connfd > stat.maxfd) {
                stat.maxfd = connfd;
            }
            if(--stat.nready <= 0) continue;
        }

        // 处理已连接client
        int curfd;
        for(int i = 0; i < stat.maxi; ++i) {
            if((curfd = stat.client[i]) < 0) {
                continue;
            }
            int n;
            if(FD_ISSET(curfd, &stat.rset)) {
                n = read(curfd, buf, MAXLINE);
                if(n == 0) {
                    close(curfd);
                    FD_CLR(curfd, &stat.rset);
                    FD_CLR(curfd, &stat.allset);
                    stat.client[i] = -1;
                }
            } else {
                write(curfd, buf, n);
                memset(buf, 0, sizeof(buf));
            }
            if(--stat.nready <= 0)
                break;
        }
    }

    exit(0);
}