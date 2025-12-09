#define NET_IMPLEMENTATION
#include "net.h"

void test_client_server() {
        int            ln, srv, client, ret, n;
        char           client_ip[IP_ADDR_STRLEN] = {};
        unsigned short client_port;
        char           server_ip[IP_ADDR_STRLEN] = {};
        unsigned short server_port;
        char           buf[64] = {};

        ln = tcplisten("0.0.0.0", 8080, TCP_LISTEN_REUSEADDR);
        if (ln < 0) {
                perror("tcplisten");
                return;
        }

        srv = tcpconnect("0.0.0.0", 8080, TCP_CONNECT_NONBLOCK);
        if (srv < 0) {
                perror("tcpdial");
                return;
        }

        client = tcpaccept(ln);
        if (client < 0) {
                perror("tcpaccept");
                return;
        }

        ret = ipaddr(client, client_ip, sizeof(client_ip), &client_port);
        if (ret < 0) {
                perror("ipaddr client");
                return;
        }
        ret = ipaddr(srv, server_ip, sizeof(server_ip), &server_port);
        if (ret < 0) {
                perror("ipaddr srv");
                return;
        }

        printf("client: %s:%d\n", client_ip, client_port);
        printf("server: %s:%d\n", server_ip, server_port);

        for (;;) {
                n = write(srv, "HELLO", 5);
                printf("client->server write:%d\n", n);
                if (n > 0)
                        break;
                usleep(10000);
        }

        for (;;) {
                n = read(client, buf, sizeof(buf));
                printf("client->server read:%d\n", n);
                if (n > 0) {
                        printf("client->server msg:%.*s\n", n, buf);
                        break;
                }
                usleep(10000);
        }

        for (;;) {
                n = write(client, "WORLD", 5);
                printf("server->client write:%d\n", n);
                if (n > 0)
                        break;
                usleep(10000);
        }

        for (;;) {
                n = read(srv, buf, sizeof(buf));
                printf("server->client read:%d\n", n);
                if (n > 0) {
                        printf("server->client msg:%.*s\n", n, buf);
                        break;
                }
                usleep(10000);
        }

        close(client);
        close(srv);
        close(ln);
}

int freeidx(int *arr, int count) {
        for (int i = 0; i < count; i++) {
                if (arr[i] == 0) {
                        return i;
                }
        }
        return -1;
}

void test_chat() {
        int           ln, ret, ev, n, idx, c;
        int           cfds[128] = {};
        struct event *e;
        struct event  events[10];

        ln = tcplisten("localhost", 8080, TCP_LISTEN_REUSEADDR | TCP_LISTEN_NONBLOCK);

        ev = evinit();
        if (ev < 0) {
                perror("evinit");
                return;
        }

        ret = evread(ev, ln, NULL);
        if (ev < 0) {
                perror("evread");
                return;
        }

        for (;;) {
                n = evwait(ev, events, 10);
                for (int i = 0; i < n; i++) {
                        e = &events[i];
                        printf("fd:%d events:%d data:%p isread:%d iswrite:%d\n", e->fd, e->events, e->data, evisread(e),
                               eviswrite(e));

                        if (e->fd == ln) {
                                c   = tcpaccept(e->fd);
                                idx = freeidx(cfds, 128);
                                if (idx < 0) {
                                        close(c);
                                } else {
                                        cfds[idx] = c;
                                        fdnonblock(c);
                                }
                                evread(ev, c, NULL);
                        } else {
                                char buf[128];
                                int  rn = read(e->fd, buf, sizeof(buf));
                                printf("%d: %.*s", e->fd, rn, buf);

                                for (int j = 0; j < 128; j++) {
                                        if (cfds[j] > 0 && cfds[j] != e->fd) {
                                                char num[8] = {};
                                                int  len    = snprintf(num, 8, "%d: ", cfds[j]);
                                                write(cfds[j], num, len);
                                                write(cfds[j], buf, rn);
                                        }
                                }
                        }
                }
        }
}

int main() {
        test_client_server();
        // test_chat();
}
