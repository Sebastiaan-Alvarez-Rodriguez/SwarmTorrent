#ifndef PEER_TORRENT_DEFAULTS_H
#define PEER_TORRENT_DEFAULTS_H

#include <chrono>
#include <cstdint>

namespace peer::defaults::torrent {
    const uint16_t max_outstanding_requests = 16;
    const auto request_stale_after_time = std::chrono::milliseconds(1500);
    const uint16_t prefered_group_size = 16;
}
#endif