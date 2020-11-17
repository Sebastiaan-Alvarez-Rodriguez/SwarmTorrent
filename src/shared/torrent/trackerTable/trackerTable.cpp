#include <algorithm>
#include <cstdint>

#include "shared/torrent/file/torrentFile.h"
#include "tracker_ip.h"

#include "trackerTable.h"

void TrackerTable::add_tracker(ConnectionType sin_family, ConnectionType socket_type, std::string addr, uint16_t sin_port) {
    struct in_addr tracker_addr;
    inet_pton(sin_family, addr.c_str(), &tracker_addr);
    add_tracker(sin_family, socket_type, tracker_addr, sin_port);
}

void TrackerTable::add_tracker(ConnectionType sin_family, ConnectionType socket_type, struct in_addr addr, uint16_t sin_port) {
    trackers.push_back(TrackerIP(sin_family, socket_type, addr, sin_port));
}

void TrackerTable::add_tracker(std::string addr, uint16_t sin_port) {
    add_tracker(AF_INET_T, SOCK_STREAM_T, addr, sin_port);
}

void TrackerTable::add_tracker(struct in_addr addr, uint16_t sin_port) {
    add_tracker(AF_INET_T, SOCK_STREAM_T, addr, sin_port);
}

void TrackerTable::remove_tracker(ConnectionType sin_family, std::string addr) {
    struct in_addr tracker_addr;
    inet_pton(sin_family, addr.c_str(), &tracker_addr);
    remove_tracker(tracker_addr);
}

void TrackerTable::remove_tracker(struct in_addr addr) {
    trackers.erase(std::remove_if(trackers.begin(), trackers.end(), [addr](const TrackerIP& t) -> bool{return t.addr.s_addr == addr.s_addr;}), trackers.end());
}

void TrackerTable::write_stream(std::ostream& os) const {
    unsigned size = trackers.size();
    os.write((char*)(&size), sizeof(size));
    for (auto tracker : trackers) 
        tracker.write_stream(os);
}

void TrackerTable::read_stream(std::istream& is) {
    unsigned size; 
    is.read((char*)(&size), sizeof(size));
    trackers.resize(size);
    for (unsigned i = 0; i < size; ++i)
        trackers[i].read_stream(is);
}  