#include "../../connection/meta/type.h"
#include <iostream>

#include <netinet/in.h>

struct Tracker_IP {
    ConnectionType sin_family;
    ConnectionType socket_type;
    struct in_addr addr;
    unsigned short sin_port;

    Tracker_IP() : sin_family(AF_INET_T), socket_type(SOCK_STREAM_T), addr({INADDR_ANY}), sin_port(0) {}
    Tracker_IP(ConnectionType _f, ConnectionType _s, struct in_addr _a, unsigned short _p) : sin_family(_f), socket_type(_s), addr(_a), sin_port(_p) {}
};

//reads and writes the Tracker_IP structure in byte format
std::ostream& operator<<(std::ostream& os, const Tracker_IP& tracker_ip);
std::istream& operator>>(std::istream& is, Tracker_IP& tracker_ip);
