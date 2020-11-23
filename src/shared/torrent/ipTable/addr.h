#ifndef TRACKERIP_H
#define TRACKERIP_H

#include <cstdint>

#include "shared/connection/meta/type.h"
#include "shared/torrent/file/streamable/streamable.h"

struct Addr : public Streamable {
    ConnectionType type;
    std::string ip;
    uint16_t port;

    Addr(ConnectionType type, std::string ip, uint16_t port) : type(type), ip(ip), port(port) {};

    // Constructs an Addr by reading from a stream
    static Addr from(std::istream& is);

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    void read_stream(std::istream& is) override;
};

#endif