#ifndef PEER_SESSION_REGISTRY_H
#define PEER_SESSION_REGISTRY_H

#include <chrono>
#include <deque>
#include <vector>
#include <unordered_map>

#include "peer/torrent/defaults.h"
#include "shared/torrent/ipTable/address.h"

namespace torrent {
    // Registry to keep track of data requests
    // it has several interesting properties:
    // 1. Can have multiple outstanding requests for the same fragments, but only to different hosts
    // 2. Insertion is constant-time
    // 3. Deletion is constant-time for removing all requests for a given fragment number
    // 4. Requests are timestamped. 'Old' requests (see peer/torrent/defaults.h request_stale_after_time)
    //    are deleted when calling [[gc()]], and is pretty slow: registered_fragments*O(log(registered_requests)+num_deleted)
    class Registry {
    protected:
        class Element;
        std::unordered_map<size_t, std::deque<Element>> requests;
        size_t total_requests = 0;

        bool gc_internal(std::deque<torrent::Registry::Element>& elems, const std::chrono::steady_clock::time_point& bound);
    public:
        Registry() = default;
        ~Registry() = default;

        // Adds request for given fragment number to given address
        void add(size_t fragment_nr, const Address& address);

        // Returns `true` if given key was found, `false` otherwise
        inline bool contains(size_t fragment_nr) const {
            return requests.find(fragment_nr) != requests.end();
        }

        // Removes all known requests for given fragment number
        void remove(size_t fragment_nr);


        /**
         * Garbage Collects all stale requests.
         * A request is considered stale when at least request_stale_after_time ms have passed.
         * This setting is found in: peer/torrent/defaults.h.
         * @return A vector containing all fragment_nrs for which there are no more requests after gc
         */
        std::vector<size_t> gc();

        /**
         * Garbage Collects all stale requests from a given fragment number
         * @return `true` if there are remaining requests for given fragment number after gc, false otherwise
         */
        bool gc(size_t fragment_nr);

        // Returns total amount of currently outstanding requests, including possible stale requests.
        // To get only non-stale requests, first use [[gc()]] to clear stale requests.
        inline size_t size() const {
            return total_requests;
        }
    };

    // One element representing a connection in progress
    class Registry::Element {
    public:
        size_t fragment_nr;
        Address address;
        std::chrono::steady_clock::time_point timestamp;
        
        Element(size_t fragment_nr, const Address& address) : fragment_nr(fragment_nr), address(address) {
            timestamp =  std::chrono::steady_clock::now();
        }
        Element() = default;

        bool operator<(const Element& other) const {
            return this->timestamp < other.timestamp;
        }
    };
}

#endif