#ifndef PEER_PIPE_OPS_H
#define PEER_PIPE_OPS_H

#include <memory>

#include "peer/torrent/session/session.h"
#include "shared/connection/connection.h"
#include "shared/connection/protocol/connections.h"

namespace peer::pipeline {
    // Handles JOIN requests
    void join(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size);

    // Handles LEAVE notifications. Remotes must send LEAVE requests from their own IPs, and must specify the registered port.
    void leave(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size);

    // Handles DATA_REQs. Closes incoming connection, reads fragment from storage, sends data to registered port for given ip.
    void data_req(torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size);

    // Handles DATA_REPLYs. Closes incoming connection, reads fragment from data, writes it to disk, and finally updates request registry.
    void data_reply(torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size);

    inline void local_discovery(const torrent::Session& session, const std::unique_ptr<ClientConnection>& connection) {
        connections::shared::send::peertable(session.get_peertable(), session.get_metadata().content_hash, message::standard::OK);
    }
}
#endif