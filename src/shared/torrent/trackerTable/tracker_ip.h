#ifndef TRACKERIP_H
#define TRACKERIP_H

#include <cstdint>
#include <iostream>
#include <netinet/in.h>

#include "shared/connection/meta/type.h"
#include "shared/torrent/file/streamable/streamable.h"

struct TrackerIP : public Streamable {
    ConnectionType sin_family;
    ConnectionType socket_type;
    struct in_addr addr;
    uint16_t sin_port;

    TrackerIP() : sin_family(AF_INET_T), socket_type(SOCK_STREAM_T), addr({INADDR_ANY}), sin_port(0) {}
    TrackerIP(ConnectionType _f, ConnectionType _s, struct in_addr _a, uint16_t _p) : sin_family(_f), socket_type(_s), addr(_a), sin_port(_p) {}

    //Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    void read_stream(std::istream& is) override;
};

#endif