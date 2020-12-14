#ifndef TRACKER_SESSION_H
#define TRACKER_SESSION_H

#include <chrono>
#include <vector>
#include <unordered_map>

#include "shared/torrent/ipTable/ipTable.h"
#include "tracker/session/registry/registry.h"

class Session {
public:
    Session() = default;

    // Get the table with hash as identifier
    inline const auto get_registry() const {
        return registry;
    }



    // registry-related forwarding functions //

    // Create a table for given hash
    inline void create_table(const std::string& hash) {
        registry.create_table(hash);
    }

    // Add a table with hash as identifier. Returns true on insertion, false when it already exists.
    inline bool add_table(const std::string& hash, IPTable&& peertable) {
        return registry.add_table(hash, std::move(peertable));
    }


    // Removes table with hash as identifier
    inline bool remove_table(const std::string& hash) {
        return registry.remove_table(hash);
    }

    // Add a peer to the table with hash as identifier. 
    // Returns true on insertion, false when IPTable not found or peer already added.
    inline bool add_peer(const std::string& hash, const Address& peer, bool exist_ok=true) {
        return registry.add_peer(hash, peer, exist_ok);
    }

    // Remove peer from table with hash as identifier
    inline bool remove_peer(const std::string& hash, const Address& peer) {
        return registry.remove_peer(hash, peer);
    }

    // Remove peer from table with hash as identifier
    inline bool remove_peer(const std::string& hash, const std::string& peer) {
        return registry.remove_peer(hash, peer);
    }

protected:
    torrent::tracker::Registry registry;
};

#endif