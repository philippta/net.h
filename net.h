#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define NET_DIAL_NONE      0x00
#define NET_DIAL_NONBLOCK  0x01
#define NET_DIAL_NODELAY   0x02
#define NET_DIAL_KEEPALIVE 0x04

#define NET_LISTEN_NONE      0x00
#define NET_LISTEN_NONBLOCK  0x01
#define NET_LISTEN_REUSEADDR 0x02
#define NET_LISTEN_REUSEPORT 0x03

#define NET_ADDR_STRLEN INET6_ADDRSTRLEN

int net_accept(int fd);
int net_addr(int fd, char *ip, size_t iplen, unsigned short *port);
int net_dial_tcp(const char *addr, unsigned short port, int flags);
int net_listen_tcp(const char *addr, unsigned short port, int flags);
int net_resolve(const char *host, struct sockaddr_in *addr);

int net_dial_tcp(const char *addr, unsigned short port, int flags) {
    int                fd, ret;
    struct sockaddr_in sockaddr = {};

    ret = net_resolve(addr, &sockaddr);
    if (ret < 0) return -1;

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(port);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    if (flags & NET_DIAL_NONBLOCK) {
        int fl = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    }

    if (flags & NET_DIAL_NODELAY) {
        int on = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    }

    if (flags & NET_DIAL_KEEPALIVE) {
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
    }

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, &on, sizeof(on));

    ret = connect(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret < 0 && errno != EINPROGRESS) {
        close(fd);
        return -1;
    }

    return fd;
}

int net_listen_tcp(const char *addr, unsigned short port, int flags) {
    int                fd, ret;
    struct sockaddr_in sockaddr = {};

    ret = net_resolve(addr, &sockaddr);
    if (ret < 0) return -1;

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(port);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    if (flags & NET_LISTEN_NONBLOCK) {
        int fl = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    }

    if (flags & NET_LISTEN_REUSEADDR) {
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    }

    if (flags & NET_LISTEN_REUSEPORT) {
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    }

    ret = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret < 0) {
        close(fd);
        return -1;
    }

    ret = listen(fd, 128);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int net_resolve(const char *host, struct sockaddr_in *addr) {
    int              ret;
    struct addrinfo  hints = {};
    struct addrinfo *res   = NULL;

    hints.ai_family = AF_INET;

    ret = getaddrinfo(host, NULL, &hints, &res);
    if (ret < 0) return -1;

    memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in));

    freeaddrinfo(res);
    return 0;
}

int net_accept(int fd) { return accept(fd, NULL, 0); }

int net_addr(int fd, char *ip, size_t iplen, unsigned short *port) {
    int                     ret;
    struct sockaddr_storage addr;
    socklen_t               addrlen;

    ret = getpeername(fd, (struct sockaddr *)&addr, &addrlen);
    if (ret < 0) return -1;

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s4 = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &s4->sin_addr, ip, iplen);
        *port = ntohs(s4->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &s6->sin6_addr, ip, iplen);
        *port = ntohs(s6->sin6_port);
    }

    return 0;
}
