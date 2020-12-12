#ifndef TRACKERIP_H
#define TRACKERIP_H

#include <cstdint>
#include <utility>

#include "shared/connection/meta/type.h"
#include "shared/torrent/file/streamable/streamable.h"

struct Address : public Streamable {
    ConnectionType type;
    std::string ip;
    uint16_t port;

    Address(ConnectionType type, std::string ip, uint16_t port) : type(type), ip(std::move(ip)), port(port) {};

    Address() : Address(ConnectionType(TransportType(), NetType()), "", 0) {}

    // Constructs an Address by reading from a stream
    static Address from(std::istream& is);

    // Constructs an Address from a string with format: TransportType:NetType:PORT:IP
    static Address from_string(std::string ip);

    // Returns the maximum size of Address, 
    // We assume ip has a maximum size of 16 bytes (IPv6)
    // Allocate 2 more bytes for NULL terminator and size of ip
    // Lastly, we need 2 bytes for the port number
    static size_t size() { return sizeof(ConnectionType) + 16 + sizeof(char) + sizeof(uint8_t) + sizeof(uint16_t);};

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    void read_stream(std::istream& is) override;

    // Read and write to a (byte) buffer
    // Return the position after read/ write
    uint8_t* write_buffer(uint8_t* const buf) const;
    const uint8_t* read_buffer(const uint8_t* const buf);
};

#endif