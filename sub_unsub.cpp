// Copyright 2023 Ilinca Sebastian-Ionut

#include "./common.h"

using namespace std;

void
unsub_from_a_topic(int cli_sock, unordered_map<string, topic_t> &topic_map,
                sub_unsub_t client_action) {
    auto it = topic_map.find(client_action.topic);
    // check topic exists
    if (it == topic_map.end()) {
        return;
    }
    // check existing subscriber
    auto itsub = it->second.subs.find(cli_sock);
    if (itsub == it->second.subs.end()) {
        return;
    }
    // verify if it wants to receive when he's back
    if (itsub->second.sf == 1) {
        sub_client_t cli_unsub(itsub->second.sf, itsub->second.socket,
                            itsub->second.mess_to_send);
        it->second.unsubs.insert(make_pair(cli_sock, cli_unsub));
    }
    it->second.subs.erase(cli_sock);
}

void
sub_to_a_topic(int cli_sock, unordered_map<string, topic_t> &topic_map,
            sub_unsub_t client_action) {
    auto it = topic_map.find(client_action.topic);

    // check topic exists
    if (it == topic_map.end()) {
        map<int, sub_client_t> new_map_sub;
        map<int, sub_client_t> new_map_unsub;
        topic_t new_topic(client_action.topic, new_map_sub, new_map_unsub);
        topic_map.insert(make_pair(client_action.topic, new_topic));
        vector<message_t> v;
        sub_client_t cli_sub(client_action.sf, cli_sock, v);
        auto new_it = topic_map.find(client_action.topic);
        new_it->second.subs.insert(make_pair(cli_sock, cli_sub));
        return;
    }

    // check if is not already subscribed
    auto is_sub_already = it->second.subs.find(cli_sock);
    if (is_sub_already != it->second.subs.end()) {
        is_sub_already->second.sf = client_action.sf;
        return;
    }
    // check if is unsubed, if not, put it in the sub
    // are without any extra work
    auto it_unsub = it->second.unsubs.find(cli_sock);
    if (it_unsub == it->second.unsubs.end()) {
        auto it_sub = it->second.subs.find(cli_sock);
        if (it_sub == it->second.subs.end()) {
            vector<message_t> v;
            sub_client_t cli_sub(client_action.sf, cli_sock, v);
            it->second.subs.insert(make_pair(cli_sock, cli_sub));
            return;
        }
        it_sub->second.sf = client_action.sf;
        return;
    }

    // send all messages for sf 1
    cout << "send all messages while he was unsubbed\n";
    for (auto comp_message : it_unsub->second.mess_to_send) {
        int rc = send_all(cli_sock, reinterpret_cast<void*>(&comp_message),
                        sizeof(comp_message));
        DIE(rc < 0, "This should not happen");
    }
    vector<message_t> v;
    sub_client_t cli_sub(client_action.sf, cli_sock, v);
    it->second.subs.insert(make_pair(cli_sock, cli_sub));
    it->second.unsubs.erase(cli_sock);
}

void
sub_unsub_client(int cli_sock, unordered_map<string, topic_t> &topic_map,
                sub_unsub_t client_action) {
    if (client_action.sub_unsub == 1) {
        sub_to_a_topic(cli_sock, topic_map, client_action);
    } else {
        unsub_from_a_topic(cli_sock, topic_map, client_action);
    }
}
