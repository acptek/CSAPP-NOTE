#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define RIO_BUFSIZE 4096
#define MAXCON 1024
#define MAXLINE 1024
#define MAXCLIENT 1024
#define PORT 4000

typedef struct {
    int rio_fd;
    int rio_cnt;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];
} rio_t;

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    int cnt;
    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0) {
            if(errno != EINTR)
                return -1;
        } else if(rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }
    cnt = n;
    if(rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;
    for(n = 1; n < maxlen; ++n) {
        if((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if(c == '\n') {
                ++n;
                break;
            }
        } else if (rc == 0) { // 读一个字符失败
            if(n == 1)
                return 0;
            else
                break;
        } else
            return -1;
    }
    *bufp = 0;
    return n-1;
}

void rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if((nwritten = write(fd, bufp, nleft)) <= 0) {
            if(errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

void echo(int connfd) {
    size_t n;
    char buf[1024];
    rio_t rio;
    rio_readinitb(&rio, connfd);
    while ((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        rio_writen(connfd, buf, n);
    }
}

void *thread (void *vargp) {
    int connfd = *((int*)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

int main(int argc, char **argv) {

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;

    pthread_t tid;
    if(argc != 2) {
        fprintf(stderr, "Usage ..\n");
        exit(0);
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in laddr;
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(strtol(argv[1], NULL, 10));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    bind(sfd, (void*)&laddr, sizeof(struct sockaddr_in));

    listen(sfd, 1024);

    while (1) {
        clientlen = sizeof(struct sockaddr_in);
        connfdp = malloc(sizeof(int));
        *connfdp = accept(sfd, (void *)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }

    exit(0);
}