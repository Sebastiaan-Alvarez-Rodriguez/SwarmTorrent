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
            std::chrono::steady_clock::time_point timestamp; // The last time we have 'seen' the peer (last time they contacted us, or we contacted them successfully)
            std::vector<bool> data_owned; // Every fragment the peer owns. Updated only once in a while. Data we provide is automatically updated.

            Element(const Address& address, std::chrono::steady_clock::time_point timestamp, const std::vector<bool>& data_owned) : address(address), timestamp(timestamp), data_owned(data_owned) {}
            Element(const Address& address, const std::vector<bool>& data_owned) : Element(address,std::chrono::steady_clock::now(), data_owned) {}
            
            Element(const Address& address, std::chrono::steady_clock::time_point timestamp, size_t num_fragments) : address(address), timestamp(timestamp), data_owned(num_fragments, false) {}
            Element(const Address& address, size_t num_fragments) : Element(address, std::chrono::steady_clock::now(), num_fragments) {}

            Element() = default;

            bool operator<(const Element& other) const {
                return this->timestamp < other.timestamp;
            }
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

        void mark(const std::string& ip);

        // Returns amount of peers in our group
        inline size_t size() const {
            return peers.size();
        }
    };
}

#endif