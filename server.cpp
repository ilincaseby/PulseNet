// Copyright 2023 Ilinca Sebastian-Ionut

#include "./common.h"

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 2) {
        printf("\n Usage: %s <port>\n", argv[0]);
        return 1;
    }
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // TCP init
    // Obtinem un socket TCP pentru receptionarea conexiunilor
    int listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0);

    // disable nagle
    int enable = 1;
    if (setsockopt(listenfd_tcp, IPPROTO_TCP, TCP_NODELAY,
                    reinterpret_cast<void*>(&enable), sizeof(enable)) < 0) {
        fprintf(stderr, "can not disable nagle\n");
    }
    DIE(listenfd_tcp < 0, "socket");
    enable = 1;
    if (setsockopt(listenfd_tcp, SOL_SOCKET, SO_REUSEADDR,
                    &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int bind_tcp = bind(listenfd_tcp, (struct sockaddr *) &addr, sizeof(addr));

    // UDP init for clients
    int listenfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr_udp;
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_port = htons(port);
    addr_udp.sin_addr.s_addr = INADDR_ANY;
    int bind_udp = bind(listenfd_udp, (struct sockaddr *) &addr_udp,
                        sizeof(addr_udp));

    // server performing
    make_majick_happen(listenfd_tcp, listenfd_udp);

    // make exit, have the presumption all clients are disconnected
    close(listenfd_tcp);
    close(listenfd_udp);
    return 0;
}
