#ifndef SHARED_CONNECTION_CACHE_H
#define SHARED_CONNECTION_CACHE_H

#include <memory>
#include <vector>

#include "shared/connection.h"

class ConnectionCache {
protected:
    // Mapping from address to cached connection
    // Note: It might be faster to use a vector
    struct CacheEntry {
        Address address;
        std::unique_ptr<ClientConnection> conn;
    };
    std::vector<CacheEntry> cache;
public:
    ConnectionCache();
    
    inline auto get(const std::string& ip) {
        return cache[ip];
    }

    // inline auto get_or_create(const std::string& ip) {
    //     auto it = cache.find(ip);
    //     if (it != cache.end())
    //         return it->second;
    //     // Create connection
    // }
};
#endif