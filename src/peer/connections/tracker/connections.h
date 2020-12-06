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
    // receive peertable from tracker
    bool receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable);
    // Update peertable to tracker
    // TODO: Probably should not be here? Soon we will need it maybe
    //bool update(std::unique_ptr<ClientConnection> connection, std::string torrent_hash, );
}
#endif