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

int byte_cnt = 0;

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

// pool中维护了活动客户端的集合
typedef struct {
    int maxfd;                  // read_set中的最大fd值
    fd_set read_set;
    fd_set ready_set;
    int nready;                 // 准备好可读数
    int maxi;
    int clientfd[FD_SETSIZE];   // 已连接fd集合，-1为可用位
    rio_t clientrio[FD_SETSIZE];// 可用的读缓冲区
} pool;

// init_pool函数初始化客户端池
/**
 * clientfd[]：已连接描述符的集合，初始化为-1，表示可用
 * 将listenfd加入到维护的客户端集合pool中
 */
void init_pool(int listenfd, pool *p) {
    int i;
    p->maxfd = -1;
    for(int i = 0; i < FD_SETSIZE; ++i)
        p->clientfd[i] = -1;
    
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

// 添加一个新的客户到活动客户池中
void add_client(int connfd, pool *p) {
    int i;
    p->nready--;
    for(int i = 0; i < FD_SETSIZE; ++i) {
        if(p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if(connfd > p->maxfd)
                p->maxfd = connfd;
            if(i > p->maxi)
                p->maxi = i;
            break;
        }
    }
    if(i == FD_SETSIZE) {
        fprintf(stderr, "add_client error: Too many clients \n");
    }
}

// check_client服务准备好的客户端连接
void check_clients (pool *p) {
    int connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for(int i = 0; (i <= p->maxi) && (p->nready > 0); ++i) {
        connfd = p->clientfd[i];

        if((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                rio_writen(connfd, buf, n);
            } else {
                close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}

int main(int argc, char **argv)
{
    static pool pool;
    if(argc != 2) {
        fprintf(stderr, "usage ..\n");
        exit(0);
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) {
        perror("socket()");
        exit(1);
    }

    struct sockaddr_in laddr, raddr;
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(strtol(argv[1], NULL, 10));
    socklen_t laddrlen = sizeof(laddr);
    inet_pton(AF_INET, "0.0.0.0", &laddrlen);
    
    if(bind(sfd, (void *)&laddr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind()");
        exit(1);
    }

    if(listen(sfd, MAXCON) < 0) {
        perror("listen()");
        exit(1);
    }

    init_pool(sfd, &pool);

    while(1) {
        pool.ready_set = pool.read_set;
        pool.nready = select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        // 处理连接请求
        if(FD_ISSET(sfd, &pool.ready_set)) {
            int clientlen = sizeof(struct sockaddr_in);
            int connfd = accept(sfd, (void *)&raddr, &clientlen);
            if(connfd < 0) {
                perror("accept()");
                exit(1);
            }
            add_client(connfd, &pool);
        }
        // 回送准备好已连接描述符的一个文本行
        check_clients(&pool);
    }
    return 0;
}

/*

select 中检测两种不同类型的输入事件：
    1 来自一个新客户端连接请求的到达
    2 一个已存在客户端的已连接描述符准备好可读
*/