// Copyright 2023 Ilinca Sebastian-Ionut

#ifndef COMMON_H__
#define COMMON_H__

#include <stddef.h>
#include <stdint.h>
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
#include <utility>
#include "./helpers.h"
#include "./defined_structs.h"
#include "./server_helper.h"

using namespace std;


int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1024

struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
};

#endif  // COMMON_H_
