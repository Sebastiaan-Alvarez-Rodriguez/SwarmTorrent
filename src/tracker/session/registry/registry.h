#ifndef TRACKER_SESSION_REGISTRY_REGISTRY_H
#define TRACKER_SESSION_REGISTRY_REGISTRY_H

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "shared/torrent/ipTable/ipTable.h"
#include "shared/torrent/ipTable/address.h"

namespace tracker::torrent {
    class Registry {
    public:
        Registry() = default;

        // Add an empty table for given hash. Returns true on insertion, false when there already is a table for hash.
        bool create_table(const std::string& hash);

        // Add a table with hash as identifier. Returns true on insertion, false when it already exists.
        bool add_table(const std::string& hash, IPTable&& peertable);

        // Find table with hash as identifier. Returns true on success, false if no such table exists.
        bool find_table(const std::string& hash, IPTable& peertable) const;

        inline const auto& get(const std::string& hash) { return peertables[hash]; }

        inline bool contains(const std::string& hash) const { return peertables.find(hash) != peertables.end(); }

        // gets all keys of registry (all hashes with a registered table)
        inline const auto get_keys() const {
            std::vector<std::string> keys;
            keys.reserve(peertables.size());
            for(const auto& kv : peertables)
                keys.push_back(kv.first);
            return keys;
        }
        inline bool has_table(const std::string& hash) const {
            return peertables.find(hash) != peertables.end();
        }

        // Get the last-checked time of a given table hash identifier
        const auto last_checked(const std::string& hash) const;

        // Removes table with hash as identifier
        bool remove_table(const std::string& hash);

        // Sets a table for a hash
        inline void set_table(const std::string& hash, IPTable&& peertable) {
            peertables[hash] = {std::move(peertable)};
        }

        inline void merge_table(const std::string& hash, IPTable&& peertable) {
            peertables[hash].table.merge(std::move(peertable));
            peertables[hash].timestamp = std::chrono::steady_clock::now();
        }

        // Add a peer to the table with hash as identifier. 
        // Returns true on insertion, false when IPTable not found or peer already added.
        bool add_peer(const std::string& hash, const Address& peer, bool exist_ok=true);

        // Remove peer from table with hash as identifier
        bool remove_peer(const std::string& hash, const Address& peer);

        // Remove peer from table with hash as identifier
        bool remove_peer(const std::string& hash, const std::string& peer);

        inline size_t size() const { return peertables.size(); }

        inline auto cbegin() const { return peertables.cbegin(); }
        inline auto cend() const { return peertables.cend(); }
    protected:
        class Element {
        public:
            Element() = default;
            Element(IPTable&& table) : timestamp(std::chrono::steady_clock::now()), table(std::move(table)) {}
            Element(const IPTable& table) : timestamp(std::chrono::steady_clock::now()), table(table) {}

            std::chrono::steady_clock::time_point timestamp; // The last time we have updated this table (useful for updating)
            IPTable table;
        };
        // Mapping from torrent hash to peertable
        std::unordered_map<std::string, Element> peertables;
    };
}
#endif