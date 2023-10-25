// Copyright 2023 Ilinca Sebastian-Ionut

#ifndef DEFINED_STRUCTS_H_
#define DEFINED_STRUCTS_H_

#include <vector>
#include <string>
#include "./common.h"

using namespace std;

struct udp_mess {
    char topic[50];
    uint8_t tip_date;
    char continut[1500];
};

struct id_tcp {
    char id_client[11];
};

struct client_t {
    id_tcp client_id;
};

struct sub_unsub_t {
    uint8_t sub_unsub;
    char topic[51];
    uint8_t sf;
};

struct message_t {
    uint32_t ip_udp; /* Saved in network order */
    uint32_t port; /* Saved in network order */
    uint8_t data_type;
    char message[1500];
    char topic[51];
    message_t(uint32_t ip, uint32_t po,
                uint8_t type, char msg[1500],
                    char top[51]) {
        ip_udp = ip;
        port = po;
        data_type = type;
        memmove(message, msg, 1500);
        memmove(topic, top, 51);
    }
    message_t() {}
};

struct sub_client_t {
    uint8_t sf;
    int socket;
    vector<message_t> mess_to_send;
    sub_client_t(uint8_t sf_o, int sock, vector<message_t> msgs) {
        sf = sf_o;
        socket = sock;
        mess_to_send = msgs;
    }
};

struct topic_t {
    char topic[51];
    map<int, sub_client_t> subs;
    map<int, sub_client_t> unsubs;
    topic_t(char t[51], map<int, sub_client_t> s, map<int, sub_client_t> u) {
        memmove(topic, t, 51);
        subs = s;
        unsubs = u;
    }
};

struct disconnected_client_t {
    string id;
    map<string, sub_client_t> unsub_sf;
    map<string, sub_client_t> sub_sf;
    vector<string> sf_0_sub;
    disconnected_client_t(string id_d, map<string, sub_client_t> unssf,
                map<string, sub_client_t> sub_sff, vector<string> sf_0_subs) {
        id = id_d;
        unsub_sf = unssf;
        sub_sf = sub_sff;
        sf_0_sub = sf_0_subs;
    }disconnected_client_t(string id_d) {
        id = id_d;
    }
};


#endif  // DEFINED_STRUCTS_H_
