#ifndef TRACKER_DEFAULTS_H
#define TRACKER_DEFAULTS_H

#include <chrono>

namespace tracker::defaults::torrent {
     const auto fast_update_time   = std::chrono::milliseconds( 60000); // amount of ms after which we will try to update the table
     const auto medium_update_time = std::chrono::milliseconds(120000); // amount of ms after which we will try to update the table
     const auto low_update_time    = std::chrono::milliseconds(300000); // amount of ms after which we will try to update the table
}
#endif