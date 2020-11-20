#ifndef CONNECTION_TYPE_H
#define CONNECTION_TYPE_H 

#include <sys/socket.h> 

// A simple start
// (TODO: Fetch integer values for types and assign them to enum vals)
// https://www.geeksforgeeks.org/socket-programming-cc/
enum NetType {
    IPv4,
    IPv6
};
inline std::ostream& operator<<(std::ostream& stream, NetType type) {
    switch (type) {
        case IPv4: stream << "IPv4"; break;
        case IPv6: stream << "IPv6"; break;
    }
    return stream;
}

enum TransportType {
    TCP
};
inline std::ostream& operator<<(std::ostream& stream, TransportType type) {
    switch (type) {
        case TCP: stream << "TCP"; break;
    }
    return stream;
}

struct ConnectionType {
    TransportType t_type;
    NetType n_type;
    ConnectionType(TransportType t_type, NetType n_type) : t_type(t_type), n_type(n_type) {}
};

inline std::ostream& operator<<(std::ostream& stream, const ConnectionType& c) {
    return stream << c.t_type <<" - " << c.n_type;
}

#endif