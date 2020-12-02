#ifndef TP_CONNECTION_PEER_H
#define TP_CONNECTION_PEER_H

#include <string>
#include <memory>

#include "shared/connection/connection.h"
#include "shared/torrent/ipTable/ipTable.h"

namespace connections {
    namespace tracker {
        // Subscribe peer to peertable of tracker
        bool subscribe(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
        // Unsubscribe peer to peertable of tracker
        bool unsubscribe(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash);
        // Recieve peertable from tracker
        bool recieve(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable);
        // Update peertable to tracker
        // TODO: Gen
        //bool update(std::unique_ptr<ClientConnection> connection, std::string torrent_hash, ); 
    }
}
#endif