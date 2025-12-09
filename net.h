#ifndef NET_H
#define NET_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define TCP_CONNECT_NONE      0x00
#define TCP_CONNECT_NONBLOCK  0x01
#define TCP_CONNECT_NODELAY   0x02
#define TCP_CONNECT_KEEPALIVE 0x04

#define TCP_LISTEN_NONE      0x00
#define TCP_LISTEN_NONBLOCK  0x01
#define TCP_LISTEN_REUSEADDR 0x02
#define TCP_LISTEN_REUSEPORT 0x03

int tcpaccept(int fd);
int tcpconnect(const char *addr, unsigned short port, int flags);
int tcplisten(const char *addr, unsigned short port, int flags);

#define IP_ADDR_STRLEN INET6_ADDRSTRLEN

int ipaddr(int fd, char *ip, size_t iplen, unsigned short *port);
int ipresolve(const char *host, struct sockaddr_in *addr);

void fdnonblock(int fd);

struct event {
        int   fd;
        int   events;
        void *data;
};

int evinit(void);
int evread(int ev, int fd, void *data);
int evwrite(int ev, int fd, void *data);
int evisread(struct event *ev);
int eviswrite(struct event *ev);
int evwait(int ev, struct event *changes, int nchanges);

#ifndef EVWAIT_KQUEUE_SIZE
#define EVWAIT_KQUEUE_SIZE 128
#endif

// #define NET_IMPLEMENTATION
#ifdef NET_IMPLEMENTATION

int tcpconnect(const char *addr, unsigned short port, int flags) {
        int                fd, ret;
        struct sockaddr_in sockaddr = {};

        ret = ipresolve(addr, &sockaddr);
        if (ret < 0)
                return -1;

        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port   = htons(port);

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
                return -1;

        if (flags & TCP_CONNECT_NONBLOCK) {
                int fl = fcntl(fd, F_GETFL, 0);
                fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        }

        if (flags & TCP_CONNECT_NODELAY) {
                int on = 1;
                setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
        }

        if (flags & TCP_CONNECT_KEEPALIVE) {
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

int tcplisten(const char *addr, unsigned short port, int flags) {
        int                fd, ret;
        struct sockaddr_in sockaddr = {};

        ret = ipresolve(addr, &sockaddr);
        if (ret < 0)
                return -1;

        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port   = htons(port);

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
                return -1;

        if (flags & TCP_LISTEN_NONBLOCK) {
                int fl = fcntl(fd, F_GETFL, 0);
                fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        }

        if (flags & TCP_LISTEN_REUSEADDR) {
                int on = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        }

        if (flags & TCP_LISTEN_REUSEPORT) {
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

int tcpaccept(int fd) {
        return accept(fd, NULL, 0);
}

int ipaddr(int fd, char *ip, size_t iplen, unsigned short *port) {
        int                     ret;
        struct sockaddr_storage addr;
        socklen_t               addrlen;

        ret = getpeername(fd, (struct sockaddr *)&addr, &addrlen);
        if (ret < 0)
                return -1;

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

int ipresolve(const char *host, struct sockaddr_in *addr) {
        int              ret;
        struct addrinfo  hints = {};
        struct addrinfo *res   = NULL;

        hints.ai_family = AF_INET;

        ret = getaddrinfo(host, NULL, &hints, &res);
        if (ret < 0)
                return -1;

        memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in));

        freeaddrinfo(res);
        return 0;
}

void fdnonblock(int fd) {
        int fl = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int evinit(void) {
        return kqueue();
}

int evread(int ev, int fd, void *data) {
        struct kevent event;
        EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, data);
        return kevent(ev, &event, 1, NULL, 0, NULL);
}

int evwrite(int ev, int fd, void *data) {
        struct kevent event;
        EV_SET(&event, fd, EVFILT_WRITE, EV_ADD, 0, 0, data);
        return kevent(ev, &event, 1, NULL, 0, NULL);
}

int evisread(struct event *ev) {
        return ev->events & EVFILT_READ & 0x01;
}

int eviswrite(struct event *ev) {
        return ev->events & EVFILT_WRITE & 0x01;
}

int evwait(int ev, struct event *changes, int nchanges) {
        int           n;
        struct kevent kevs[EVWAIT_KQUEUE_SIZE];

        if (nchanges > EVWAIT_KQUEUE_SIZE) {
                nchanges = EVWAIT_KQUEUE_SIZE;
        }

        n = kevent(ev, NULL, 0, kevs, nchanges, NULL);
        if (n < 0) {
                return n;
        }

        for (int i = 0; i < n; i++) {
                changes[i].fd     = kevs[i].ident;
                changes[i].events = kevs[i].filter;
                changes[i].data   = kevs[i].udata;
        }
        return n;
}

#endif /* NET_IMPLEMENTATION */
#endif /* NET_H */
