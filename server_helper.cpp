// Copyright 2023 Ilinca Sebastian-Ionut

#include "./server_helper.h"
#include "./common.h"

using namespace std;

#define MAX_CONNS 128

void
allign_w_others(id_tcp client_id, unordered_map<string, int> &client_map,
                unordered_map<int, string> &socket_client, int sock_cli) {
    client_map.insert(make_pair(client_id.id_client, sock_cli));
    socket_client.insert(make_pair(sock_cli, client_id.id_client));
}

void
shut_down_conn_client(struct pollfd poll_fds[MAX_CONNS], int *conns_so_far,
                int index, unordered_map<string, int> &client_map,
                unordered_map<int, string> &socket_client,
                unordered_map<string, disconnected_client_t> &disc_clients,
                unordered_map<string, topic_t> &topic_map) {
    int sock_cli = poll_fds[index].fd;
    auto it = socket_client.find(sock_cli);
    if (it != socket_client.end()) {
        string id = socket_client[sock_cli];
        cout << "Client " << id << " disconnected.\n";

        // Retain if is sub to something and if it is willing to receive
        // notifications when he is back
        disconnected_client_t d_client(id);
        for (auto topic = topic_map.begin();
            topic != topic_map.end(); ++topic) {
            auto iter = topic->second.subs.find(sock_cli);
            if (iter != topic->second.subs.end()) {
                if (iter->second.sf == 0) {
                    d_client.sf_0_sub.push_back(topic->first);
                } else if (iter->second.sf == 1) {
                    d_client.sub_sf.insert(make_pair
                                (topic->first, iter->second));
                }
                topic->second.subs.erase(sock_cli);
            }
            auto unsub_iter = topic->second.unsubs.find(sock_cli);
            if (unsub_iter != topic->second.unsubs.end()) {
                d_client.unsub_sf.insert(make_pair
                                (topic->first, unsub_iter->second));
                topic->second.unsubs.erase(sock_cli);
            }
        }

        // add if I have something to tell him when he comes back
        if (!d_client.sf_0_sub.empty() || !d_client.sub_sf.empty()
            || !d_client.unsub_sf.empty()) {
            disc_clients.insert(make_pair(id, d_client));
        }
        socket_client.erase(it);
        auto itera = client_map.find(id);
        if (itera != client_map.end()) {
            client_map.erase(itera);
        }
    }
    for (int i = index; i < *conns_so_far - 1; ++i) {
        poll_fds[i] = poll_fds[i + 1];
    }
    *conns_so_far = *conns_so_far - 1;
    close(sock_cli);
}

void
new_client_add_up(struct pollfd poll_fds[MAX_CONNS], int *conns_so_far,
                int index, unordered_map<string, int> &client_map,
                unordered_map<int, string> &socket_client,
                unordered_map<string, disconnected_client_t> &disc_clients,
                unordered_map<string, topic_t> &topic_map) {
    // accept connection
    struct sockaddr_in cli_addr;
    socklen_t socklen = sizeof(cli_addr);
    int sock_cli = accept(poll_fds[index].fd,
                    (struct sockaddr *) &cli_addr, &socklen);
    int enable = 1;
    if (setsockopt(sock_cli, IPPROTO_TCP, TCP_NODELAY,
        reinterpret_cast<char *>(&enable), sizeof(enable)) < 0) {
        fprintf(stderr, "can not disable nagle");
    }

    id_tcp client_id;

    // receive id
    int rc = recv_all(sock_cli, &client_id, sizeof(client_id));
    DIE(rc < 0, "This should not happen, can not receive client id");
    int garbage = 2;
    // search for the same id
    if (client_map.size() == 0) {
        rc = send_all(sock_cli, reinterpret_cast<void*>(&garbage),
                    sizeof(garbage));
        DIE(rc < 0, "Can't send confirmation");
        allign_w_others(client_id, client_map, socket_client, sock_cli);
    } else if (client_map.size() != 0) {
        if (client_map.find(client_id.id_client) != client_map.end()) {
            cout << "Client " << client_id.id_client << " already connected.\n";
            close(sock_cli);
            return;
        }
        rc = send_all(sock_cli, reinterpret_cast<void*>(&garbage),
                    sizeof(garbage));
        DIE(rc < 0, "Can't send confirmation");
        allign_w_others(client_id, client_map, socket_client, sock_cli);
    }

    char *ip_str = inet_ntoa(cli_addr.sin_addr);

    cout << "New client " << client_id.id_client
        << " connected from " << ip_str << ":"
        << ntohs(cli_addr.sin_port) << "\n";

    // remake the subs as it was before leaving
    auto it = disc_clients.find(client_id.id_client);
    if (it != disc_clients.end()) {
        for (auto comp : it->second.sf_0_sub) {
            vector<message_t> v;
            sub_client_t aux(0, sock_cli, v);
            topic_map.find(comp)->second.subs.insert(make_pair(sock_cli, aux));
        }
        for (auto comp = it->second.sub_sf.begin();
            comp != it->second.sub_sf.end(); ++comp) {
            comp->second.socket = sock_cli;
            for (auto msg : comp->second.mess_to_send) {
                rc = send_all(sock_cli, reinterpret_cast<void*>(&msg),
                            sizeof(msg));
                DIE(rc < 0, "Should not");
            }
            vector<message_t> v;
            sub_client_t aux(1, sock_cli, v);
            topic_map.find(comp->first)->second.
                    subs.insert(make_pair(sock_cli, aux));
        }
        for (auto comp = it->second.unsub_sf.begin();
            comp != it->second.unsub_sf.end(); ++comp) {
            comp->second.socket = sock_cli;
            topic_map.find(comp->first)->second.
                    unsubs.insert(make_pair(sock_cli, comp->second));
        }
        disc_clients.erase(it);
    }

    // add in poll the new client
    poll_fds[*conns_so_far].fd = sock_cli;
    poll_fds[*conns_so_far].events = POLLIN;
    poll_fds[*conns_so_far].revents = 0x000;
    *conns_so_far = *conns_so_far + 1;
}

void
close_all(struct pollfd poll_fds[MAX_CONNS], int conns_so_far) {
    for (int i = 3; i < conns_so_far; ++i) {
        close(poll_fds[i].fd);
    }
}

void make_majick_happen(int listenfd_tcp, int listenfd_udp) {
    // initiate poll and first 3 fds
    struct pollfd poll_fds[MAX_CONNS];
    int conns_so_far = 0;
    int rc = listen(listenfd_tcp, MAX_CONNS);
    DIE(rc < 0, "listen_problems");
    poll_fds[conns_so_far].fd = STDIN_FILENO;
    poll_fds[conns_so_far].events = POLLIN;
    conns_so_far++;
    poll_fds[conns_so_far].fd = listenfd_tcp;
    poll_fds[conns_so_far].events = POLLIN;
    conns_so_far++;
    poll_fds[conns_so_far].fd = listenfd_udp;
    poll_fds[conns_so_far].events = POLLIN;
    conns_so_far++;
    unordered_map<string, int> client_map;
    unordered_map<int, string> socket_client;
    unordered_map<string, topic_t> topic_map;
    unordered_map<string, disconnected_client_t> disc_clients;

    // server loop
    while (1) {
        rc = poll(poll_fds, conns_so_far, -1);
        DIE(rc < 0, "poll");
        for (int i = 0; i < conns_so_far; ++i) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == listenfd_tcp) {
                    // treat new connections
                    new_client_add_up(poll_fds, &conns_so_far,
                                i, client_map, socket_client,
                                disc_clients, topic_map);
                    continue;
                } else if (poll_fds[i].fd == STDIN_FILENO) {
                    // treat input
                    char buf[20];
                    rc = read(STDIN_FILENO, buf, sizeof(buf));
                    DIE(rc < 0, "Can not read from stdin");
                    if (buf[strlen(buf) - 1] == '\n') {
                        buf[strlen(buf) - 1] = '\0';
                    }
                    if (strncmp(buf, "exit", 4) == 0) {
                        close_all(poll_fds, conns_so_far);
                        return;
                    } else {
                        fprintf(stderr, "unknown command\n");
                    }
                    continue;

                } else if (poll_fds[i].fd == listenfd_udp) {
                    // treat udp
                    treat_udp(poll_fds[i].fd, topic_map, disc_clients);
                    continue;
                } else {
                    // treat commands from clients
                    sub_unsub_t client_action;
                    rc = recv_all(poll_fds[i].fd, &client_action,
                                sizeof(client_action));
                    DIE(rc < 0, "Something wrong");
                    if (rc == 0) {
                        shut_down_conn_client(poll_fds,
                                    &conns_so_far, i, client_map,
                                    socket_client, disc_clients, topic_map);
                    }
                    if (rc > 0) {
                        sub_unsub_client(poll_fds[i].fd,
                                        topic_map, client_action);
                    }
                }
            }
        }
    }
}
