#ifndef CONNECTION_TYPE_H
#define CONNECTION_TYPE_H 

#include <cstdint>
#include <ostream>
#include <sys/socket.h> 

class NetType {
public:
    // Enum for supported NetType
    enum Type : uint8_t {
        IPv4,
        IPv6
    };
    Type t = IPv4;

    NetType() = default;
    NetType(Type t) : t(t) {}

    inline bool operator==(const NetType& other) const { 
        return this->t == other.t; 
    }

    friend std::ostream& operator<<(std::ostream& stream, const NetType& t);

    // Returns the ctype of the NetTypes type 
    inline int to_ctype() const {
        return t == IPv4 ? AF_INET : AF_INET6;
    }

};

inline std::ostream& operator<<(std::ostream& stream, const NetType& t) {
    switch (t.t) {
        case NetType::IPv4: stream << "IPv4"; break;
        case NetType::IPv6: stream << "IPv6"; break;
    }
    return stream;
}


class TransportType {
public:
    // Enum for supported TransportTypes
    enum Type {
        TCP
    };
    Type t = TCP;

    TransportType() = default;
    TransportType(Type t) : t(t) {}

    // Returns the ctype of the TransportTypes type
    inline int to_ctype() const {
        return SOCK_STREAM;
    }

    inline bool operator==(const TransportType& other) const { 
        return this->t == other.t; 
    }

    inline bool operator==(const Type& other) const { 
        return this->t == other; 
    }

    inline bool operator!=(const Type& other) const { 
        return !(*this == other); 
    }

    friend std::ostream& operator<<(std::ostream& stream, const TransportType& t);

};
inline std::ostream& operator<<(std::ostream& stream, const TransportType& t) {
    switch (t.t) {
        case TransportType::TCP: stream << "TCP"; break;
    }
    return stream;
}


struct ConnectionType {
    TransportType t_type;
    NetType n_type;

    ConnectionType(TransportType t_type, NetType n_type) : t_type(t_type), n_type(n_type) {}
    ConnectionType(TransportType::Type t, NetType::Type n) : t_type(t), n_type(n) {}
    ConnectionType() = default;

    inline bool operator==(const ConnectionType& other) { 
        return this->t_type == other.t_type && this->n_type == other.n_type;
    }

};

inline std::ostream& operator<<(std::ostream& stream, const ConnectionType& c) {
    return stream << c.t_type <<" - " << c.n_type;
}



#endif