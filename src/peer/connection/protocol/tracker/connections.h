#ifndef TP_CONNECTION_PEER_TRACKER_H
#define TP_CONNECTION_PEER_TRACKER_H

#include <string>
#include <memory>

#include "shared/connection/connection.h"
#include "shared/torrent/ipTable/ipTable.h"

namespace connections::tracker {
    // Send a simple test message to tracker
    bool test(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
    
    namespace send {
        // Register new peertable on a tracker
        bool make_torrent(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
    
        /**
         * Register this peer at tracker.
         *
         * '''Note:''' Only initial peers need to do this.
         */
        bool register_self(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port);

        // Send a RECEIVE request to a tracker
        bool receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
    }

    namespace recv {
        // Receive peertable from tracker
        bool receive(std::unique_ptr<ClientConnection>& connection, IPTable& peertable, Address& own_address, uint16_t sourcePort);
    }
}
#endif