#ifndef PEER_TORRENT_DEFAULTS_H
#define PEER_TORRENT_DEFAULTS_H

#include <chrono>
#include <cstdint>

namespace peer::defaults::torrent {
    const uint16_t outgoing_requests = 200; // prefered number of active requests for data
    const auto request_stale_after_time = std::chrono::milliseconds(1500); // amount of ms before a request is considered stale
    const uint16_t prefered_group_size = 16; //amount of peers we would like to have in our group
    const uint16_t prefered_known_peers_size = 64; // amount of peers we want to know about

}
#endif