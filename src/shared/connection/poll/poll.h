#ifndef SHARED_CONNECTION_POLL_H
#define SHARED_CONNECTION_POLL_H


#include <arpa/inet.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

#include "shared/connection/cache/cache.h"
#include "shared/connection/connection.h"

class Poll {
public:
    // Poll on clientconnections only
    void do_poll(const std::vector<std::shared_ptr<ClientConnection>>& connections, size_t timeout_ms, std::function<bool(std::shared_ptr<ClientConnection>)> cache_func);

    // Poll on a hostconnection and any ClientConnections
    void do_poll(const std::unique_ptr<HostConnection>& hostconnection, const std::vector<std::shared_ptr<ClientConnection>>& connections, size_t timeout_ms, std::function<bool(std::shared_ptr<ClientConnection>)> listen_func, std::function<bool(std::shared_ptr<ClientConnection>)> cache_func);

protected:
    
};
#endif