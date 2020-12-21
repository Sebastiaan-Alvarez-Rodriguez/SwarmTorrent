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
    std::unordered_map<Address, std::shared_ptr<ClientConnection>> cache;
    mutable std::shared_mutex mutex;
public:
    ConnectionCache() = default;

    // Returns the connection associated with given address from the cache
    inline auto get(const Address& address) {
        std::shared_lock lock(mutex);
        return cache[address];
    }

    // Returns the connection associated with given addres from the cache as an optional
    inline auto get_optional(const Address& address) {
        std::shared_lock lock(mutex);
        auto it = cache.find(address);
        return it == cache.end() ? std::nullopt : std::optional<std::shared_ptr<ClientConnection>>{it->second};
    }

    // Returns if cache contains connection associated with address
    inline bool contains(const Address& address) {
        std::shared_lock lock(mutex);
        return cache.find(address) != cache.end();
    }

    // Inserts new connection associated with given address
    inline bool insert(const Address& address, std::unique_ptr<ClientConnection>&& conn) {
        std::unique_lock lock(mutex);
        return cache.insert({address, std::move(conn)}).second;
    }

    // Inserts new connection associated with given address
    inline bool insert(const Address& address, std::shared_ptr<ClientConnection>& conn) {
        std::unique_lock lock(mutex);
        return cache.insert({address, conn}).second;
    }

    // Erases connection associated with given address
    inline void erase(const Address& address) {
        std::unique_lock lock(mutex);
        cache.erase(address);
    }

    // Returns all cache keys
    inline auto keys() {
        std::shared_lock lock(mutex);
        std::vector<Address> ans;
        ans.reserve(cache.size());
        for (const auto&[key, val] : cache)
            ans.push_back(key);
        return ans;
    }

    // Returns all cache values
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