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
#include <bits/stdc++.h>
#include <unordered_map>
#include "./common.h"
#include "./helpers.h"
#include "./defined_structs.h"
#include "./server_helper.h"

using namespace std;

int
index_null_terminator(char buf[1551], char topic[51]) {
    topic[50] = '\0';
    for (int i = 0; i < 50; ++i) {
        topic[i] = buf[i];
        if (buf[i] == '\0')
            return i + 1;
    }
    return 50;
}

void treat_udp(int listenfd_udp, unordered_map<string, topic_t> &topic_map,
            unordered_map<string, disconnected_client_t> &disc_clients) {
    struct sockaddr_in cli_addr;
    socklen_t socklen = sizeof(cli_addr);
    char buf[1551];
    int rc = recvfrom(listenfd_udp, reinterpret_cast<void*>(buf),
                    sizeof(buf), 0, (struct sockaddr *) &cli_addr, &socklen);
    DIE(rc < 0, "This should not happen <udp receive>");
    char topic[51];
    topic[50] = '\0';
    memmove(topic, buf, 50);
    auto it = topic_map.find(topic);
    if (it == topic_map.end()) {
        return;
    }
    uint8_t data_type;
    memmove(&data_type, buf + 50, sizeof(data_type));
    // initiate message
    char containt[1500];
    memmove(containt, buf + 51, rc - 51);
    if (rc < 1551) {
        containt[rc - 51] = '\0';
    }
    message_t message_to_send(cli_addr.sin_addr.s_addr,
                            cli_addr.sin_port, data_type,
                            containt, topic);

    // in a for loop send to all subs the news
    // use const as it is not modified in the process
    for (const auto& pair : it->second.subs) {
        rc = send_all(pair.second.socket,
        reinterpret_cast<void*>(&message_to_send), sizeof(message_to_send));
        DIE(rc < 0, "This should not happen");
    }

    // in a for loop store the message
    for (auto pair = it->second.unsubs.begin();
        pair != it->second.unsubs.end(); ++pair) {
        pair->second.mess_to_send.push_back(message_to_send);
    }

    // send messages to people who are disconnected if they wanna to
    for (auto it = disc_clients.begin(); it != disc_clients.end(); ++it) {
        auto sub_sf = it->second.sub_sf.find(topic);
        if (sub_sf != it->second.sub_sf.end()) {
            sub_sf->second.mess_to_send.push_back(message_to_send);
            continue;
        }
        auto unsub_sf = it->second.unsub_sf.find(topic);
        if (unsub_sf != it->second.unsub_sf.end()) {
            unsub_sf->second.mess_to_send.push_back(message_to_send);
        }
    }
}
