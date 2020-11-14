#include <cstdint>

#include "tracker_ip.h"

//note, should ConnectionType be expanend, 
//then the ternary statements should change to switch statements
static uint8_t pack(ConnectionType sin_family, ConnectionType socket_type) {
    uint8_t return_code = 0b00000000;
    uint8_t MASK1 = (sin_family == AF_INET_T) ? 0b00000000 : 0b00000001;
    uint8_t MASK2 = (socket_type == SOCK_STREAM_T) ? 0b00000000 : 0b00000010;
    return return_code | MASK1 | MASK2;
}

//note, should ConnectionType be expanend, then the ternary statements should change 
static void unpack(ConnectionType* sin_family, ConnectionType* socket_type, uint8_t byte) {
    uint8_t MASK1 = 0b00000001;
    uint8_t MASK2 = 0b00000010;
    *sin_family = (byte & MASK1) ? AF_INET6_T : AF_INET_T;
    *socket_type = ((byte & MASK2) >> 1) ? SOCK_DGRAM_T : SOCK_STREAM_T;
}

std::ostream& operator<<(std::ostream& os, const Tracker_IP& tracker_ip) {
    os << pack(tracker_ip.sin_family, tracker_ip.socket_type);
    os.write((char*)(&tracker_ip.addr.s_addr), sizeof(tracker_ip.addr.s_addr));
    os.write((char*)(&tracker_ip.sin_port), sizeof(tracker_ip.sin_port));
    return os;
}

std::istream& operator>>(std::istream& is, Tracker_IP& tracker_ip) {
    uint8_t byte; 
    is >> byte;
    unpack(&tracker_ip.sin_family, &tracker_ip.socket_type, byte);
    is.read((char*)(&tracker_ip.addr.s_addr), sizeof(tracker_ip.addr.s_addr));
    is.read((char*)(&tracker_ip.sin_port), sizeof(tracker_ip.sin_port));
    return is;
}