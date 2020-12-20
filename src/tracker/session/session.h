#ifndef TRACKER_SESSION_H
#define TRACKER_SESSION_H

#include <chrono>
#include <optional>
#include <random>
#include <shared_mutex>
#include <vector>
#include <unordered_map>

#include "shared/util/random/randomGenerator.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "tracker/session/registry/registry.h"

class Session {
public:
    Session() = default;

    // Get the table with hash as identifier
    inline const auto& get_registry_unsafe() const {
        return registry;
    }

    inline const auto get_registry_keys() const {
        std::shared_lock lock(mutex);
        return registry.get_keys();
    }

    // registry-related forwarding functions //


    // Add an empty table for given hash. Returns true on insertion, false when there already is a table for hash.
    inline bool registry_add_for(const std::string& hash) {
        std::unique_lock lock(mutex);
        return registry.create_table(hash);
    }

    // Add a table with hash as identifier. Returns true on insertion, false when it already exists.
    inline bool registry_add_table(const std::string& hash, IPTable&& peertable) {
        std::unique_lock lock(mutex);
        return registry.add_table(hash, std::move(peertable));
    }

    // Removes table with hash as identifier
    inline bool registry_remove_table(const std::string& hash) {
        std::unique_lock lock(mutex);
        return registry.remove_table(hash);
    }

    // Returns table for given hash as an optional. If no such table exists, optional is empty.
    inline std::optional<IPTable> registry_find_table(const std::string& hash) const {
        std::shared_lock lock(mutex);
        IPTable table;
        return registry.find_table(hash, table) ? std::optional<IPTable>{table} : std::nullopt;
    }

    inline bool registry_contains(const std::string& hash) const { 
        std::shared_lock lock(mutex);
        return registry.contains(hash);
    }

    // Sets a table for a hash
    inline void registry_set_table(const std::string& hash, IPTable&& peertable) {
        std::unique_lock lock(mutex);
        registry.set_table(hash, std::move(peertable));
    }

    inline const auto& registry_element_for_unsafe(const std::string& hash) { return registry.get(hash); } 

    inline const auto registry_element_for(const std::string& hash) {
        std::shared_lock lock(mutex);
        return registry.get(hash);
    }

    // Add a peer to the table with hash as identifier. 
    // Returns true on insertion, false when IPTable not found or peer already added.
    inline bool registry_add_peer(const std::string& hash, const Address& peer, bool exist_ok=true) {
        std::unique_lock lock(mutex);
        return registry.add_peer(hash, peer, exist_ok);
    }

    // Remove peer from table with hash as identifier
    inline bool registry_remove_peer(const std::string& hash, const Address& peer) {
        std::unique_lock lock(mutex);
        return registry.remove_peer(hash, peer);
    }

    // Remove peer from table with hash as identifier
    inline bool registry_remove_peer(const std::string& hash, const std::string& peer) {
        std::unique_lock lock(mutex);
        return registry.remove_peer(hash, peer);
    }

protected:
    tracker::torrent::Registry registry;
    mutable std::shared_mutex mutex;
};

#endif