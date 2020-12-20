#include <cstring>
#include <cerrno>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <vector>
#include <sys/poll.h>
#include <sys/socket.h>

#include "shared/util/print.h"
#include "poll.h"

static void prepare_pollfds(struct pollfd* ptr, const std::vector<std::shared_ptr<ClientConnection>>& connections) {
    for (size_t x = 0; x < connections.size(); ++x) {
        ptr[x].fd = connections[x]->getfd();
        ptr[x].events = POLLIN;
    }
}


void Poll::do_poll(const std::vector<std::shared_ptr<ClientConnection>>& connections, size_t timeout_ms, std::function<bool(std::shared_ptr<ClientConnection>)> cache_func) {
    struct pollfd pfds[200];

    std::memset(pfds, 0, sizeof(struct pollfd)*connections.size());

    prepare_pollfds(pfds, connections);

    std::cerr << "Polling for " << (connections.size()) << " connections (no HostConnection)\n";
    int poll_ans = poll(pfds, connections.size(), timeout_ms);

    if (poll_ans < 0) {
        std::cerr << print::RED << "Poll: Error while trying to poll!\n" << print::CLEAR;
        exit(1);
    } else if (poll_ans == 0) {
        // std::cerr << print::YELLOW << "Poll: timeout reached. Continuing...\n" << print::CLEAR;
    } else {
        for (size_t x = 0; x < connections.size(); ++x) {
            int revents = pfds[x].revents;
            if (revents == 0)
                continue;
            if (revents != POLLIN) {
                std::cerr << print::RED << "Had an error for fd " << connections[x]->getfd() << " (location " << x << " in vector)\n" << print::CLEAR;
                continue;
            }
            std::cerr << print::CYAN << "fd " << connections[x] << " is readable now!\n" << print::CLEAR;

            if (!cache_func(connections[x])) {//TODO: remove this from cached cache
            }
        }
    }
}
// Poll on a hostconnection and any ClientConnections
void Poll::do_poll(const std::unique_ptr<HostConnection>& hostconnection, const std::vector<std::shared_ptr<ClientConnection>>& connections, size_t timeout_ms, std::function<bool(std::shared_ptr<ClientConnection>)> listen_func, std::function<bool(std::shared_ptr<ClientConnection>)> cache_func) {
    // struct pollfd pfds[] = new struct pollfd[fds.size()];
    struct pollfd pfds[200];

    std::memset(pfds, 0, sizeof(struct pollfd)*(1+connections.size()));
    pfds[0].fd = hostconnection->getfd();
    pfds[0].events = POLLIN;
    prepare_pollfds(&pfds[1], connections);

    // std::cerr << "Polling for " << (1+connections.size()) << " connections (1 is HostConnection)\n";
    int poll_ans = poll(pfds, 1+connections.size(), timeout_ms);

    if (poll_ans < 0) {
        std::cerr << print::RED << "Poll: Error while trying to poll!\n" << print::CLEAR;
        exit(1);
    } else if (poll_ans == 0) {
        // std::cerr << print::YELLOW << "Poll: timeout reached. Continuing...\n"<< print::CLEAR;
    } else {
        for (size_t x = 0; x < 1+connections.size(); ++x) {
            int revents = pfds[x].revents;
            if (revents == 0) // not a triggered socket
                continue;
            if (revents != POLLIN) { // We got a returnevent that is not POLLIN (posibly remote hangup. Error!)
                std::cerr << print::RED << "Had an error for fd " << connections[x-1]->getfd() << print::CLEAR;
                if (x == 0) {
                    hostconnection->print(std::cerr);
                } else {
                    std::cerr << " (location " << (x-1) << " in vector) "; connections[x-1]->print(std::cerr);
                }
                std::cerr << ' ' << std::strerror(errno) << '\n';
                continue;
            }

            // std::cerr << print::CYAN << "An fd is readable now! " << print::CLEAR;
            if (x == 0) { // this is the listening hostconnection
                // std::cerr << "It is the HostConnection!\n";
                std::shared_ptr<ClientConnection> new_conn = std::move(hostconnection->acceptConnection());
                new_conn->setBlocking(true);
                if (listen_func(new_conn)) {//TODO: add this to cached cache
                }
            } else { // this is some other connection from the cache
                // std::cerr << "It is a ClientConnection!\n";
                if (!cache_func(connections[x-1])) {
                    //TODO: remove this from cached cache
                }
            }
        }
    }
}