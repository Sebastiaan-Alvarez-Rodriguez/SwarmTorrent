#ifndef SHARED_CONNECTION_CACHE_H
#define SHARED_CONNECTION_CACHE_H

#include <memory>
#include <optional>
#include <unordered_map>
#include <shared_mutex>

#include "shared/connection/connection.h"
#include "shared/torrent/ipTable/address.h"

class ConnectionCache {
protected:
    // Mapping from address to cached connection
    // Note: It might be faster to use a vector
    // struct CacheEntry {
    //     Address address;
    //     std::unique_ptr<ClientConnection> conn;
    // };
    std::unordered_map<Address, std::shared_ptr<ClientConnection>> cache;
    mutable std::shared_mutex mutex;
public:
    ConnectionCache() = default;

    inline auto get(const Address& address) {
        std::shared_lock lock(mutex);
        return cache[address];
    }

    inline auto get_optional(const Address& address) {
        std::shared_lock lock(mutex);
        auto it = cache.find(address);
        return it == cache.end() ? std::nullopt : std::optional<std::shared_ptr<ClientConnection>>{it->second};
    }

    inline bool contains(const Address& address) {
        std::shared_lock lock(mutex);
        return cache.find(address) != cache.end();
    }

    inline bool insert(const Address& address, std::unique_ptr<ClientConnection>&& conn) {
        std::unique_lock lock(mutex);
        return cache.insert({address, std::move(conn)}).second;
    }

    inline bool insert(const Address& address, std::shared_ptr<ClientConnection>& conn) {
        std::unique_lock lock(mutex);
        return cache.insert({address, conn}).second;
    }

    inline void erase(const Address& address) {
        std::unique_lock lock(mutex);
        cache.erase(address);
    }

    inline auto keys() {
        std::shared_lock lock(mutex);
        std::vector<Address> ans;
        ans.reserve(cache.size());
        for (const auto&[key, val] : cache)
            ans.push_back(key);
        return ans;
    }

    inline auto values() {
        std::shared_lock lock(mutex);
        std::vector<std::shared_ptr<ClientConnection>> ans;
        ans.reserve(cache.size());
        for (const auto&[key, val] : cache)
            ans.push_back(val);
        return ans;
    }
};
#endif