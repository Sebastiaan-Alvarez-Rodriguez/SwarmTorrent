#ifndef TRACKERIP_H
#define TRACKERIP_H

#include <cstdint>
#include <utility>

#include "shared/connection/meta/type.h"
#include "shared/torrent/file/streamable/streamable.h"

/** 
 * Base implementation to hold addresses. This implementation is made for TCP/IP addresses.
 * This means that address 1 == address 2 if their ip's and their ports match.
 */
struct Address : public Streamable {
    ConnectionType type;
    std::string ip;
    uint16_t port;

    Address(ConnectionType type, std::string&& ip, uint16_t port) : type(type), ip(std::move(ip)), port(port) {};
    Address(ConnectionType type, const std::string& ip, uint16_t port) : type(type), ip(ip), port(port) {};
 
    Address(std::string&& ip, uint16_t port) : type({TransportType(), NetType()}), ip(std::move(ip)), port(port) {}
    Address(const std::string& ip, uint16_t port) : type({TransportType(), NetType()}), ip(ip), port(port) {}

    Address() : Address(ConnectionType(TransportType(), NetType()), "", 0) {}

    // Constructs an Address by reading from a stream
    static Address from(std::istream& is);

    // Constructs an Address from a string with format: TransportType:NetType:PORT:IP
    static Address from_string(std::string& ip);

    inline bool operator==(const Address& other) const {
        return this->ip == other.ip && this->port == other.port;
    }


    // We enforce ip has a maximum size of 16 bytes (for IPv6, IPv4 uses less)
    constexpr static size_t ip_size() { 
        return 16; 
    }

    // Returns the size of an Address, applying ip constant expression enforcement.
    // We allocate first the connectiontype, then the ip buffer, next up 1 more byte for size of ip.
    // Lastly, we need 2 bytes for the port number
    constexpr static size_t size() { 
        return sizeof(ConnectionType) + ip_size()*sizeof(char) + sizeof(uint8_t) + sizeof(uint16_t); 
    }

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    void read_stream(std::istream& is) override;

    // Read and write to a (byte) buffer
    // Return the position after read/ write
    uint8_t* write_buffer(uint8_t* const buf) const;
    const uint8_t* read_buffer(const uint8_t* const buf);
};

#include "shared/util/anticollision.h"

namespace std {
    template <>
    struct hash<Address> {
        size_t operator()(const Address& a) const {
            size_t val_a = std::hash<std::string>()(a.ip);
            anticollision::boost(val_a, std::hash<uint16_t>()(a.port));
            return val_a;
        }
    };

}



#endif