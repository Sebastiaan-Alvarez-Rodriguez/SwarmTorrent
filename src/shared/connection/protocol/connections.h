#ifndef SHARED_CONNECTION_PROTOCOL_CONNECTIONS_H
#define SHARED_CONNECTION_PROTOCOL_CONNECTIONS_H

#include <memory>

#include "shared/connection/connection.h"
#include "shared/connection/message/message.h"
#include "shared/torrent/ipTable/ipTable.h"

namespace connections::shared {
    namespace send {
        // Send a discovery request to other end, with given hash.
        // In shared namespace, as both a tracker and any peer can issue local discoveries.
        bool discovery_req(const std::shared_ptr<ClientConnection>& connection, const std::string& hash);

        // Send a peertable to other end in response to a LOCAL_DISCOVERY request.
        // '''Note:''' Also sends own address
        bool discovery_reply(const std::shared_ptr<ClientConnection>& connection, const IPTable& peertable, const std::string& hash, const Address& addr);
    }

    namespace recv {
        // Read a DISCOVERY_REQ
        bool discovery_req(const uint8_t* const data, size_t size, std::string& hash);

        // Receives a discovery reply from other end. It should contain first the hash for the table, then the peertable itself.
        bool discovery_reply(const uint8_t* const data, size_t size, IPTable& peertable, std::string& hash);
    }
}

#endif