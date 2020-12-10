#ifndef TP_CONNECTION_PEER_TRACKER_H
#define TP_CONNECTION_PEER_TRACKER_H

#include <string>
#include <memory>

#include "shared/connection/connection.h"
#include "shared/torrent/ipTable/ipTable.h"

namespace connections::tracker {
    // Send a simple test message to tracker
    bool test(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
    
    // Register new peertable on a tracker
    bool make_torrent(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
    
    // Receive peertable from tracker
    bool receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable);

    /**
     * Register this peer at tracker.
     *
     * '''Note:''' TODO: Soon, only initial peers need to do this.
     */
    bool register_self(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port);

    // Update peertable to tracker
    // TODO: Probably should not be here? Soon we will need it maybe
    //bool update(std::unique_ptr<ClientConnection> connection, std::string torrent_hash, );
}
#endif