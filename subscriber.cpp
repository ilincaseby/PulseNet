// Copyright 2023 Ilinca Sebastian-Ionut

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <netinet/tcp.h>
#include <bits/stdc++.h>
#include <unordered_map>
#include "./common.h"
#include "./helpers.h"
#include "./defined_structs.h"
#include "./server_helper.h"


int
interpret_commands(sub_unsub_t &sub_commands, char command[128]) {
    char buffer[128];
    memmove(buffer, command, 128);
    char *token = strtok(buffer, " \n");
    if (token == NULL) {
        return -1;
    }
    if (strncmp(token, "exit", 4) == 0) {
        return 0;
    } else if (strncmp(token, "subscribe", 9) == 0) {
        sub_commands.sub_unsub = 1;
        token = strtok(NULL, " \n");
        if (token == NULL) {
            return -1;
        }
        if (strlen(token) > 50) {
            return -1;
        }
        memmove(sub_commands.topic, token, strlen(token) + 1);
        token = strtok(NULL, " \n");
        if (token == NULL) {
            return -1;
        }
        uint8_t sf = -1;
        int rc = sscanf(token, "%hhu", &sf);
        DIE(rc != 1, "Can not identify sf\n");
        sub_commands.sf = sf;
        return 1;
    } else if (strncmp(token, "unsubscribe", 11) == 0) {
        sub_commands.sub_unsub = 0;
        token = strtok(NULL, " \n");
        if (token == NULL) {
            return -1;
        }
        memset(sub_commands.topic, 0, 51);
        memmove(sub_commands.topic, token, strlen(token));
        return 1;
    }
    return -1;
}

void
show_news(message_t &msg) {
    struct in_addr addr;
    addr.s_addr = msg.ip_udp;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
    cout << ip_str << ":" << ntohs(msg.port) << " - " << msg.topic << " - ";
    if (msg.data_type == 0) {
        cout << "INT - ";
        uint8_t sign;
        memmove(&sign, msg.message, 1);
        uint32_t number;
        memmove(&number, msg.message + 1, sizeof(number));
        number = ntohl(number);
        if (sign == 0) {
            cout << number << "\n";
        } else {
            cout << "-" << number << "\n";
        }
        return;
    } else if (msg.data_type == 1) {
        cout << "SHORT_REAL - ";
        uint16_t number;
        memmove(&number, msg.message, sizeof(number));
        number = ntohs(number);
        float no = (static_cast<float> (number)) / 100;
        cout << fixed << setprecision(2) << no << "\n";
        return;
    } else if (msg.data_type == 2) {
        cout << "FLOAT - ";
        uint8_t sign;
        memmove(&sign, msg.message, 1);
        uint32_t number;
        memmove(&number, msg.message + 1, sizeof(number));
        number = ntohl(number);
        uint8_t power;
        memmove(&power, msg.message + 5, sizeof(power));
        if (sign == 1) {
            cout << "-";
        }
        uint8_t power_copy = power;
        uint32_t number_copy = number;
        while (power_copy > 0) {
            number_copy /= 10;
            power_copy--;
        }
        cout << number_copy << ".";
        power_copy = power;
        number_copy = number;
        char str[300];
        str[299] = '\0';
        int index = 299;
        while (power_copy > 0) {
            index--;
            power_copy--;
            str[index] = 48 + (number_copy % 10);
            number_copy /= 10;
        }
        cout << str + index << "\n";
    } else if (msg.data_type == 3) {
        cout << "STRING - ";
        char str[1501];
        str[1500] = '\0';
        memmove(str, msg.message, sizeof(msg.message));
        cout << str << "\n";
    }
}

void
fill_client_with_news(int sockfd) {
    message_t msg;
    sub_unsub_t sub_commands;
    struct pollfd poll_fds[2];
    int num_clients = 0;

    // add stdin
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    num_clients++;

    // add socket to communicate with server
    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN;
    num_clients++;

    char buf[128] = {};

    while (1) {
        int rc = poll(poll_fds, num_clients, -1);
        DIE(rc < 0, "poll error");
        for (int i = 0; i < num_clients; ++i) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == STDIN_FILENO) {
                    // TREAT COMMANDS
                    memset(buf, 0, 128);
                    rc = read(STDIN_FILENO, buf, sizeof(buf));
                    DIE(rc < 0, "read failed!\n");
                    int interpreted = interpret_commands(sub_commands, buf);
                    if (interpreted == 1) {
                        // client wants to un/subscribe
                        rc = send_all(sockfd,
                                    reinterpret_cast<void*>(&sub_commands),
                                    sizeof(sub_commands));
                        DIE(rc < 0, "Can not send from client to server\n");
                        if (sub_commands.sub_unsub == 1) {
                            cout << "Subscribed to topic.\n";
                        } else {
                            cout << "Unsubscribed from topic.\n";
                        }
                    } else if (interpreted == 0) {
                        // client wants to close connection
                        return;
                    } else if (interpreted == -1) {
                        fprintf(stderr, "UNKNOWN COMMAND\n");
                    }
                } else if (poll_fds[i].fd == sockfd) {
                    // TREAT NEWS
                    rc = recv_all(sockfd, reinterpret_cast<void*>(&msg),
                                    sizeof(msg));
                    DIE(rc < 0, "Can not receive from server");
                    if (rc == 0) {
                        return;
                    }
                    show_news(msg);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sockfd = -1;
    if (argc != 4) {
        printf("\n Usage: %s <id_client> <ip_server> <port_server>\n", argv[0]);
        return 1;
    }
    if (strlen(argv[1]) > 10) {
        fprintf(stderr, "Error: ID should not exceed 10 characters\n");
        exit(0);
    }
    char my_id[11];
    memset(my_id, 0, sizeof(my_id));
    memmove(my_id, argv[1], strlen(argv[1]));

    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port); /* Warning! is in host order */
    DIE(rc != 1, "Given port is not ok");
    uint16_t port_nt = htons(port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "can not open socket");

    // Disable Nagle
    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
        &enable, sizeof(enable)) < 0) {
        fprintf(stderr, "Can not disable nagle\n");
        exit(0);
    }

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port_nt;
    rc = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
    DIE(rc < 0,
    "Error: Internal function is not working or ip address is not correct");

    rc = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connection gone wrong");

    rc = send_all(sockfd, reinterpret_cast<void*>(my_id), sizeof(my_id));

    // After this, we'll receive a packet, it could be an int or nothing
    int garb;
    rc = recv_all(sockfd, reinterpret_cast<void*>(&garb), sizeof(garb));
    DIE(rc < 0, "Problem with recv_all client");

    if (rc == 0) {
        close(sockfd);
        return 0;
    }

    fill_client_with_news(sockfd);

    close(sockfd);
    return 0;
}
