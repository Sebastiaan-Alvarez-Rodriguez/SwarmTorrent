#ifndef SHARED_CONNECTION_PROTOCOL_CONNECTIONS_H
#define SHARED_CONNECTION_PROTOCOL_CONNECTIONS_H

#include <memory>

#include "shared/connection/connection.h"
#include "shared/connection/message/message.h"
#include "shared/torrent/ipTable/ipTable.h"

namespace connections::shared {
    namespace send {
        // Send a peertable to other end, together with given standard tag
        bool peertable(const std::unique_ptr<ClientConnection>& connection, const IPTable& peertable, const std::string& hash, message::standard::Tag tag);
    }

    namespace recv {
        // Receives a peertable from other end.
        bool peertable(const uint8_t* const data, size_t size, IPTable& peertable, std::string& hash);
    }
}

#endif