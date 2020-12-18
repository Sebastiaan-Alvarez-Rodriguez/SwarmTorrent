#ifndef PEER_TORRENT_DEFAULTS_H
#define PEER_TORRENT_DEFAULTS_H

#include <chrono>
#include <cstdint>

namespace peer::torrent::defaults {
    constexpr const uint16_t outgoing_requests = 200; // prefered number of active requests for data
    constexpr const auto request_stale_after_time = std::chrono::milliseconds(1500); // amount of ms before a request is considered stale
    constexpr const uint16_t prefered_group_size = 16; //amount of peers we would like to have in our group
    constexpr const uint16_t prefered_known_peers_size = 64; // amount of peers we want to know about
    constexpr const uint16_t inactive_threshold = 10; // allowed number of unresponsiveness to requests from a peer, before it is set inactive
    constexpr const auto availability_update_time = std::chrono::milliseconds(10000); // amount of ms before a request is considered stale
    constexpr const auto dead_torrent_poke_time = std::chrono::milliseconds(10000); // amount of ms to wait between poking tracker when peer seems to be dead & we have no peers.
}
#endif