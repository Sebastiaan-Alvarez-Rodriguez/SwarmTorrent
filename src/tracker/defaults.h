#ifndef TRACKER_DEFAULTS_H
#define TRACKER_DEFAULTS_H

#include <chrono>

namespace tracker::torrent::defaults {
     const auto fast_update_time     = std::chrono::milliseconds( 30000); // amount of ms after which we will try to update the table when table is small
     const size_t fast_update_size   = 10; // When there are less than this many peers in peertable, perform fast updates
     const auto medium_update_time   = std::chrono::milliseconds(120000); // amount of ms after which we will try to update the table when table is medium
     const size_t medium_update_size = 32; // When there are less than this many peers in peertable, perform fast updates
     const auto slow_update_time     = std::chrono::milliseconds(300000); // amount of ms after which we will try to update the table when table is large
     const auto discovery_tick_time  = std::chrono::milliseconds(  6000); // amount of ms between every discovery check
     const size_t fast_update_pool_size   =  5;
     const size_t medium_update_pool_size =  8;
     const size_t slow_update_pool_size   = 12;
}
#endif