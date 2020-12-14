#include <algorithm>
#include <chrono>
#include <deque>
#include <vector>
#include <unordered_map>

#include "peer/torrent/defaults.h"
#include "shared/torrent/ipTable/address.h"
#include "registry.h"


bool torrent::request::Registry::gc_internal(std::deque<torrent::request::Registry::Element>& elems, const std::chrono::steady_clock::time_point& bound) {
    Element e;
    e.timestamp = bound;
    // const auto it = std::lower_bound(elems.begin(), elems.end(), e, [&](const auto& point1) {});
    const auto it = std::lower_bound(elems.begin(), elems.end(), e);
    if (it != elems.end()) { // There are some stale items
        const auto size_before = elems.size();
        elems.erase(elems.begin(), it);
        total_requests -= (size_before - elems.size());
    }
    return elems.size() > 0;
}

void torrent::request::Registry::add(size_t fragment_nr, const Address& address) {
    // requests.push_back({fragment_nr, {fragment_nr, address}});

    requests[fragment_nr].push_back(torrent::request::Registry::Element(fragment_nr, address));
    ++total_requests;
    // if (!requests.emplace(fragment_nr, std::deque<torrent::Registry::Element> e).second)
    //     requests[fragment_nr].push_back({fragment_nr, {fragment_nr, address}});
}

void torrent::request::Registry::remove(size_t fragment_nr) {
    const auto size = requests[fragment_nr].size();
    requests.erase(fragment_nr);
    total_requests -= size;
}

bool torrent::request::Registry::gc(size_t fragment_nr) {
    const auto now = std::chrono::steady_clock::now();
    return gc_internal(requests[fragment_nr], now-peer::defaults::torrent::request_stale_after_time);
}

inline std::vector<size_t> torrent::request::Registry::gc() {
    auto vec = std::vector<size_t>();
    const auto now = std::chrono::steady_clock::now();
    for (auto& [key, val] : requests)
        if (!gc_internal(val, now-peer::defaults::torrent::request_stale_after_time))
            vec.push_back(key);
    return vec;
}