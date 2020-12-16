#ifndef PEER_SESSION_PEERS_REGISTRY_H
#define PEER_SESSION_PEERS_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>

#include "peer/torrent/defaults.h"
#include "shared/torrent/ipTable/address.h"

namespace peer::torrent {
    // Registry to keep track of peers in our group
    // it has several interesting properties:
    // 1. Can have multiple outstanding requests for the same fragments, but only to different hosts
    // 2. Insertion is constant-time
    // 3. Deletion is constant-time for removing all requests for a given fragment number
    // 4. Requests are timestamped. 'Old' requests (see peer/torrent/defaults.h request_stale_after_time)
    //    are deleted when calling [[gc()]], and is pretty slow: registered_fragments*O(log(registered_requests)+num_deleted)

    class PeerRegistry {
    protected:
        // One element representing a connection in progress
        struct Element {
            std::vector<bool> data_owned; // Every fragment the peer owns. Updated only once in a while. Data we provide is automatically updated.
            unsigned inactiveCounter = 0; // Counter to keep track of the unresponsiveness of a peer

            Element(const std::vector<bool>& data_owned) : data_owned(data_owned) {}
            Element(size_t num_fragments) : data_owned(num_fragments, false) {}
            Element() = default;
        };
        // a mapping from address to owned data, inactiveCounter
        std::unordered_map<Address, Element> peers;

    public:
        // Adds request for given fragment number to given address
        inline void add(Address&& address, std::vector<bool>&& fragments_completed) {
            peers.insert({std::move(address), std::move(fragments_completed)});
        }
        inline void add(Address& address, std::vector<bool>&& fragments_completed) {
            peers.insert({address, std::move(fragments_completed)});
        }
        inline void add(Address&& address, std::vector<bool>& fragments_completed) {
            peers.insert({std::move(address), fragments_completed});
        }
        inline void add(const Address& address, const std::vector<bool>& fragments_completed) {
            peers.insert({address, peer::torrent::PeerRegistry::Element(fragments_completed)});
        }

        // Returns `true` if given key was found, `false` otherwise
        inline bool contains(const Address& address) const {
            return peers.find(address) != peers.end();
        }

        // Removes all known requests for given fragment number
        inline void remove(const Address& address) { peers.erase(address); }

        // Returns vector of addresses which own given fragment (and belong to our group of course)
        std::vector<Address> get_peers_for(size_t fragment_nr) const;

        inline void update_peer_fragments(const Address& address, std::vector<bool>&& updated) {
            if (!contains(address))
                return;
            peers[address].data_owned = std::move(updated);
        }
        // Resets the inactiveCounter of the peer
        void mark(const Address& address);
        // Reports the peer as being unresponsive
        void report(const Address& address);

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