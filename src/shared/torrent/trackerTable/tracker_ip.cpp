#include <cstdint>

#include "tracker_ip.h"

//Note, should ConnectionType be expanend, 
//then the ternary statements should change to switch statements
static uint8_t pack(ConnectionType sin_family, ConnectionType socket_type) {
    uint8_t return_code = 0b00000000;
    uint8_t MASK1 = (sin_family == AF_INET_T) ? 0b00000000 : 0b00000001;
    uint8_t MASK2 = (socket_type == SOCK_STREAM_T) ? 0b00000000 : 0b00000010;
    return return_code | MASK1 | MASK2;
}

//Note, should ConnectionType be expanend, then the ternary statements should change 
static void unpack(ConnectionType* sin_family, ConnectionType* socket_type, uint8_t byte) {
    uint8_t MASK1 = 0b00000001;
    uint8_t MASK2 = 0b00000010;
    *sin_family = (byte & MASK1) ? AF_INET6_T : AF_INET_T;
    *socket_type = ((byte & MASK2) >> 1) ? SOCK_DGRAM_T : SOCK_STREAM_T;
}

void TrackerIP::write_stream(std::ostream& os) const {
    os << pack(sin_family, socket_type);
    os.write((char*)(&addr.s_addr), sizeof(addr.s_addr));
    os.write((char*)(&sin_port), sizeof(sin_port));
}

void TrackerIP::read_stream(std::istream& is) {
    uint8_t byte; 
    is >> byte;
    unpack(&sin_family, &socket_type, byte);
    is.read((char*)(&addr.s_addr), sizeof(addr.s_addr));
    is.read((char*)(&sin_port), sizeof(sin_port));
}