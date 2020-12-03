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

    // Constructs an Address by reading from a stream
    static Address from(std::istream& is);

    // Constructs an Address from a string with format: TransportType:NetType:PORT:IP
    static Address from_string(std::string ip);

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    void read_stream(std::istream& is) override;
};

#endif