#ifndef TRACKER_DEFAULTS_H
#define TRACKER_DEFAULTS_H

#include <chrono>

namespace tracker::torrent::defaults {
     constexpr const auto fast_update_time     = std::chrono::milliseconds( 30000); // amount of ms after which we will try to update the table when table is small
     constexpr const size_t fast_update_size   = 10; // When there are less than this many peers in peertable, perform fast updates
     constexpr const auto medium_update_time   = std::chrono::milliseconds(120000); // amount of ms after which we will try to update the table when table is medium
     constexpr const size_t medium_update_size = 32; // When there are less than this many peers in peertable, perform fast updates
     constexpr const auto slow_update_time     = std::chrono::milliseconds(300000); // amount of ms after which we will try to update the table when table is large
     constexpr const auto discovery_tick_time  = std::chrono::milliseconds(  6000); // amount of ms between every discovery check
     constexpr const size_t fast_update_pool_size   =  5;
     constexpr const size_t medium_update_pool_size =  8;
     constexpr const size_t slow_update_pool_size   = 12;
}
#endif