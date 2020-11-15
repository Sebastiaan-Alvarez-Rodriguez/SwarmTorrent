#include <algorithm>

#include "trackerTable.h"

void TrackerTable::add_tracker(ConnectionType sin_family, ConnectionType socket_type, std::string tracker_ip, unsigned short sin_port) {
    struct in_addr tracker_addr;
    inet_aton(tracker_ip.c_str(), &tracker_addr);
    add_tracker(sin_family, socket_type, tracker_addr, sin_port);
}

void TrackerTable::add_tracker(ConnectionType sin_family, ConnectionType socket_type, struct in_addr tracker_ip, unsigned short sin_port) {
    trackers.push_back(Tracker_IP(sin_family, socket_type, tracker_ip, sin_port));
}

void TrackerTable::add_tracker(std::string tracker_ip, unsigned short sin_port) {
    add_tracker(AF_INET_T, SOCK_STREAM_T, tracker_ip, sin_port);
}

void TrackerTable::add_tracker(struct in_addr tracker_ip, unsigned short sin_port) {
    add_tracker(AF_INET_T, SOCK_STREAM_T, tracker_ip, sin_port);
}

void TrackerTable::remove_tracker(std::string tracker_ip) {
    struct in_addr tracker_addr;
    inet_aton(tracker_ip.c_str(), &tracker_addr);
    remove_tracker(tracker_addr);
}

void TrackerTable::remove_tracker(struct in_addr tracker_ip) {
    trackers.erase(std::remove_if(trackers.begin(), trackers.end(), [tracker_ip](const Tracker_IP& t) -> bool{return t.addr.s_addr == tracker_ip.s_addr;}), trackers.end());
}

void TrackerTable::write_swarm(std::ostream& os) {
    unsigned size = trackers.size();
    os.write((char*)(&size), sizeof(size));
    for (auto tracker : trackers) 
        tracker.write_swarm(os);
}

void TrackerTable::read_swarm(std::istream& is) {
    unsigned size; 
    is.read((char*)(&size), sizeof(size));
    trackers.resize(size);
    for (unsigned i = 0; i < size; ++i)
        trackers[i].read_swarm(is);
}  