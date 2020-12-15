#ifndef PEER_SESSION_PEERS_REGISTRY_H
#define PEER_SESSION_PEERS_REGISTRY_H

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include "peer/torrent/defaults.h"
#include "shared/torrent/ipTable/address.h"

namespace torrent::peer {
    // Registry to keep track of peers in our group
    // it has several interesting properties:
    // 1. Can have multiple outstanding requests for the same fragments, but only to different hosts
    // 2. Insertion is constant-time
    // 3. Deletion is constant-time for removing all requests for a given fragment number
    // 4. Requests are timestamped. 'Old' requests (see peer/torrent/defaults.h request_stale_after_time)
    //    are deleted when calling [[gc()]], and is pretty slow: registered_fragments*O(log(registered_requests)+num_deleted)

    class Registry {
    protected:
        // One element representing a connection in progress
        struct Element {
            Address address;
            std::vector<bool> data_owned; // Every fragment the peer owns. Updated only once in a while. Data we provide is automatically updated.
            unsigned inactiveCounter = 0; // Counter to keep track of the unresponsiveness of a peer

            Element(const Address& address, const std::vector<bool>& data_owned) : address(address), data_owned(data_owned) {}
            Element(const Address& address, size_t num_fragments) : address(address), data_owned(num_fragments, false) {}
            Element() = default;
        };
        std::unordered_map<std::string, Element> peers;

    public:
        Registry() = default;
        ~Registry() = default;

        // Adds request for given fragment number to given address
        void add(const Address& address, const std::vector<bool> fragments_completed);

        // Returns `true` if given key was found, `false` otherwise
        inline bool contains(const std::string& ip) const {
            return peers.find(ip) != peers.end();
        }

        // Removes all known requests for given fragment number
        inline void remove(const std::string& ip) { peers.erase(ip); }
        inline void remove(const Address& address) { remove(address.ip); }

        inline void update_peer_fragments(const std::string& ip, std::vector<bool>&& updated) {
            if (!contains(ip))
                return;
            peers[ip].data_owned = std::move(updated);
        }
        // Resets the inactiveCounter of the peer
        void mark(const std::string& ip);
        // Reports the peer as being unresponsive
        void report(const std::string& ip);

        // Performs a garbage collect to remove unresponsive peers
        void gc();

        inline auto cbegin() const { return peers.cbegin(); }
        inline auto cend() const { return peers.cend(); }

        // Returns amount of peers in our group
        inline size_t size() const {
            return peers.size();
        }
    };
}

#endif