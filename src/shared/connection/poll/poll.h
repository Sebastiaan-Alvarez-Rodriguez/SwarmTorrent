#ifndef SHARED_CONNECTION_POLL_H
#define SHARED_CONNECTION_POLL_H


#include <arpa/inet.h>
#include <cstdint>
#include <memory>
#include <vector>

#include "shared/connection/cache/cache.h"
#include "shared/connection/connection.h"

// Next step: Make connections/sends/recvs with a timeout, or maybe even non-blocking?
//  https://stackoverflow.com/questions/4181784/
// Quick post about using non-blocking connections sensibly:
//  https://jameshfisher.com/2017/04/05/set_socket_nonblocking/
// Next next step: polling system for accepting connections?
//  https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm
// Doing stuff in a very hard way with select:
//  https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
// Both polling and selecting are very hardcore things to implement, and neither is pretty.

class Poll {
public:
    // Poll on clientconnections only
    void do_poll(const std::vector<std::shared_ptr<ClientConnection>>& connections, size_t timeout_ms, std::function<bool(std::shared_ptr<ClientConnection>)> cache_func);

    // Poll on a hostconnection and any ClientConnections
    void do_poll(const std::unique_ptr<HostConnection>& hostconnection, const std::vector<std::shared_ptr<ClientConnection>>& connections, size_t timeout_ms, std::function<bool(std::shared_ptr<ClientConnection>)> listen_func, std::function<bool(std::shared_ptr<ClientConnection>)> cache_func);

protected:
    
};
#endif