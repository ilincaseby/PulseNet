// Copyright 2023 Ilinca Sebastian-Ionut

#ifndef SERVER_HELPER_H_
#define SERVER_HELPER_H_

#include <unordered_map>
#include <string>
#include "./common.h"

using namespace std;

void make_majick_happen(int listenfd_tcp, int listenfd_udp);
void sub_unsub_client(int cli_sock, unordered_map<string,
                    topic_t> &topic_map, sub_unsub_t client_action);
void treat_udp(int listenfd_udp,
                unordered_map<string, topic_t> &topic_map,
                unordered_map<string, disconnected_client_t> &disc_clients);

#endif  // SERVER_HELPER_H_
