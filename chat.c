#define NET_IMPLEMENTATION
#include "net.h"

int freeidx(int *arr, int count) {
        for (int i = 0; i < count; i++) {
                if (arr[i] == 0) {
                        return i;
                }
        }
        return -1;
}

int main(void) {
        int  cfds[128] = {};
        ev_t events[10];

        int ln = tcplisten("localhost", 8080, TCP_LISTEN_REUSEADDR | TCP_LISTEN_NONBLOCK);
        if (ln < 0) {
                perror("tcplisten");
                return 0;
        }

        printf("listening on localhost:8080\n");

        int ev = evinit();
        if (ev < 0) {
                perror("evinit");
                return 0;
        }

        int ret = evread(ev, ln, NULL);
        if (ev < 0) {
                perror("evread");
                return 0;
        }

        evloop(ev, {
                if (ev_fd == ln) {
                        int c   = tcpaccept(ev_fd);
                        int idx = freeidx(cfds, 128);
                        if (idx < 0) {
                                close(c);
                        } else {
                                cfds[idx] = c;
                                fdnonblock(c);
                        }
                        evread(ev, c, (void *)(uintptr_t)idx);
                        printf("%d: joined\n", idx);
                        for (int j = 0; j < 128; j++) {
                                if (cfds[j] > 0 && j != idx) {
                                        char num[16] = {};
                                        int  len     = snprintf(num, 16, "%d: joined\n", idx);
                                        write(cfds[j], num, len);
                                }
                        }
                } else {
                        int  idx = (int)(uintptr_t)ev_data;
                        char buf[128];
                        int  rn = read(ev_fd, buf, sizeof(buf));
                        if (rn <= 0) {
                                close(ev_fd);
                                cfds[idx] = 0;
                                printf("%d: left\n", idx);

                                for (int j = 0; j < 128; j++) {
                                        if (cfds[j] > 0 && cfds[j] != ev_fd) {
                                                char num[16] = {};
                                                int  len     = snprintf(num, 16, "%d: left\n", idx);
                                                write(cfds[j], num, len);
                                        }
                                }
                        } else {
                                printf("%d: %.*s", idx, rn, buf);

                                for (int j = 0; j < 128; j++) {
                                        if (cfds[j] > 0 && cfds[j] != ev_fd) {
                                                char num[8] = {};
                                                int  len    = snprintf(num, 8, "%d: ", idx);
                                                write(cfds[j], num, len);
                                                write(cfds[j], buf, rn);
                                        }
                                }
                        }
                }
        });
}
