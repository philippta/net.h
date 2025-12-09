#define NET_IMPLEMENTATION
#include "net.h"

int main(void) {
        int            ln, srv, client, ret, n;
        char           client_ip[IP_ADDR_STRLEN] = {};
        unsigned short client_port;
        char           server_ip[IP_ADDR_STRLEN] = {};
        unsigned short server_port;
        char           buf[64] = {};

        ln = tcplisten("0.0.0.0", 8080, TCP_LISTEN_REUSEADDR);
        if (ln < 0) {
                perror("tcplisten");
                return 0;
        }

        srv = tcpconnect("0.0.0.0", 8080, TCP_CONNECT_NONBLOCK);
        if (srv < 0) {
                perror("tcpdial");
                return 0;
        }

        client = tcpaccept(ln);
        if (client < 0) {
                perror("tcpaccept");
                return 0;
        }

        ret = ipaddr(client, client_ip, sizeof(client_ip), &client_port);
        if (ret < 0) {
                perror("ipaddr client");
                return 0;
        }
        ret = ipaddr(srv, server_ip, sizeof(server_ip), &server_port);
        if (ret < 0) {
                perror("ipaddr srv");
                return 0;
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
