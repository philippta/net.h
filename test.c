#include "net.h"

void test_client_server() {
    int            ln, srv, client, ret, n;
    char           client_ip[NET_ADDR_STRLEN] = {};
    unsigned short client_port;
    char           server_ip[NET_ADDR_STRLEN] = {};
    unsigned short server_port;
    char           buf[64] = {};

    ln = net_listen_tcp("0.0.0.0", 8080, NET_LISTEN_REUSEADDR);
    if (ln < 0) {
        perror("net_listen_tcp");
        return;
    }

    srv = net_dial_tcp("0.0.0.0", 8080, NET_DIAL_NONBLOCK);
    if (srv < 0) {
        perror("net_dial_tcp");
        return;
    }

    client = net_accept(ln);
    if (client < 0) {
        perror("net_accept");
        return;
    }

    ret = net_addr(client, client_ip, sizeof(client_ip), &client_port);
    if (ret < 0) {
        perror("net_addr client");
        return;
    }
    ret = net_addr(srv, server_ip, sizeof(server_ip), &server_port);
    if (ret < 0) {
        perror("net_addr srv");
        return;
    }

    printf("client: %s:%d\n", client_ip, client_port);
    printf("server: %s:%d\n", server_ip, server_port);

    for (;;) {
        n = write(srv, "HELLO", 5);
        printf("client->server write:%d\n", n);
        if (n > 0) break;
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
        if (n > 0) break;
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

int main() { test_client_server(); }
