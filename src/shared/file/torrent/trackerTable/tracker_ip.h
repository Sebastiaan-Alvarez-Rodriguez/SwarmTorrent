#include <iostream>
#include <netinet/in.h>

#include "../../../connection/meta/type.h"
#include "../swarmTorrentWriter.h"

struct Tracker_IP : public SwarmTorrentWriter {
    ConnectionType sin_family;
    ConnectionType socket_type;
    struct in_addr addr;
    unsigned short sin_port;

    Tracker_IP() : sin_family(AF_INET_T), socket_type(SOCK_STREAM_T), addr({INADDR_ANY}), sin_port(0) {}
    Tracker_IP(ConnectionType _f, ConnectionType _s, struct in_addr _a, unsigned short _p) : sin_family(_f), socket_type(_s), addr(_a), sin_port(_p) {}

    //Read and write to a SwarmTorrent file
    void write_swarm(std::ostream& os) override;
    void read_swarm(std::istream& is) override;
};

