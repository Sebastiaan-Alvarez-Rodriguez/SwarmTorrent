#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>


#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "shared/connection/meta/type.h"
#include "shared/connection/connection.h"
#include "shared/util/socket.h"
#include "generic/generic.h"

//TODO: https://www.geeksforgeeks.org/explicitly-assigning-port-number-client-socket/
// Need to explicitly bind to sourcePort for client connections?
class TCPClientConnection : public ClientConnection {
public:
    class Factory;

    explicit inline TCPClientConnection(ConnectionType type, const std::string& address, uint16_t  sourcePort, uint16_t destinationPort, bool blockmode, bool reusemode) : ClientConnection::ClientConnection(type, address, sourcePort, destinationPort, blockmode, reusemode) {
        this->state = ClientConnection::ERROR;
        if ((this->sockfd = sock::make(type)) < 0) {
            std::cerr << "Could not build socket!" << std::endl;
            return;
        }

        if (!blockmode && !sock::set_blocking(this->sockfd, false)) {
            std::cerr << "Could not set blockmode to false\n";
            return;
        }

        if (reusemode && !sock::set_reuse(this->sockfd)) {
            std::cerr << "Could not set reusemode to true\n";
            return;
        }

        struct sockaddr_in addr_in;
        addr_in.sin_family = type.n_type.to_ctype();
        addr_in.sin_port = htons(destinationPort);

        if(inet_pton(type.n_type.to_ctype(), address.c_str(), &addr_in.sin_addr) <= 0) {
            std::cerr << "Could not assign address!" << std::endl;
            return;
        }
        this->sock_addr = *(struct sockaddr*) &addr_in;

        if (sourcePort > 0) {
            // Convention says we should explicitly bind to the user-specified local port
            struct sockaddr_in own_addr;
            own_addr.sin_family = type.n_type.to_ctype(); 
            own_addr.sin_addr.s_addr = INADDR_ANY; 
            own_addr.sin_port = htons(sourcePort); 
            if (!bind(this->sockfd, (struct sockaddr*) &own_addr, sizeof(struct sockaddr_in)) == 0) 
                throw std::runtime_error("Could not make connection have port "+sourcePort);
        }
        this->state = READY;
    }

    explicit inline TCPClientConnection(ConnectionType type, const std::string& address, uint16_t sourcePort, uint16_t destinationPort, bool blockmode, bool reusemode, int sockfd, struct sockaddr sock_addr): ClientConnection::ClientConnection(type, address, sourcePort, destinationPort, blockmode, reusemode), sockfd(sockfd), sock_addr(sock_addr) {
        this->state = CONNECTED;
    }

     ~TCPClientConnection() {
        close(sockfd);
    }

    inline bool doConnect() override {
        if (connect(sockfd, &sock_addr, sizeof(sockaddr_in)) >= 0) {
            this->state = CONNECTED;
            return true;
        }
        return false;
    }

    inline bool sendmsg(const uint8_t* const msg, unsigned length, int flags) const override {
        return tcp::sendmsg(sockfd, msg, length, flags);
    }

    inline bool recvmsg(uint8_t* const msg, unsigned length, int flags) const override {
        return tcp::recvmsg(sockfd, msg, length, flags);
    }

    inline bool peekmsg(uint8_t* const msg, unsigned length, int flags) const override {
        return tcp::peekmsg(sockfd, msg, length, flags);
    }

    inline virtual bool discardmsg(__attribute__ ((unused)) unsigned length) const {
        return recvmsg(nullptr, 0, MSG_TRUNC);
    };

    inline virtual bool setBlocking(bool blockmode) {
        return blockmode == this->blockmode || sock::set_blocking(this->sockfd, blockmode);
    }

    inline void print(std::ostream& stream) const override {
        stream << "Client target:" << type << ": " << address << ':' << sourcePort;
    }

protected:
    int sockfd;
    struct sockaddr sock_addr;
};

class TCPHostConnection: public HostConnection {
public:
    class Factory;

    explicit inline TCPHostConnection(ConnectionType type, uint16_t sourcePort, bool blockmode, bool reusemode) : HostConnection(type, sourcePort, blockmode, reusemode) {
        this->state = ClientConnection::ERROR;
        if ((this->sockfd = sock::make(type)) < 0) {
            std::cerr << "Could not build socket!" << std::endl;
            return;
        }

        if (!blockmode && !sock::set_blocking(this->sockfd, false)) {
            std::cerr << "Could not set blockmode to false\n";
            return;
        }

        if (reusemode && !sock::set_reuse(this->sockfd)) {
            std::cerr << "Could not set reusemode to true\n";
            return;
        }

        struct sockaddr_in* addr_in = (struct sockaddr_in*) &(this->sock_addr);
        std::memset((uint8_t*)addr_in, 0, sizeof(sockaddr_in));
        // bzero((char *) &sock, sizeof(sock));
        addr_in->sin_family = this->type.n_type.to_ctype();
        addr_in->sin_addr.s_addr = INADDR_ANY;
        addr_in->sin_port = htons(sourcePort);

        if(bind(sockfd, (struct sockaddr*) addr_in, sizeof(sockaddr_in)) < 0) {
            std::cerr << "Could not bind socket! Maybe address already in use?" << std::endl;
            return;
        }

        if (listen(sockfd, 10) < 0) { //TODO: Change hardcoded 10 for connection backlog to a global constant somewhere
            //TODO: Check if this listen to socket could return < 0 on non-blocking sockets, while being alright.
            //      If so, we should fix it.
            // if (this->blockmode/* || (!this->blockmode && errno != EWOULDBLOCK)*/) {
            //     this->state = ERROR;
            //     std::cerr << "Could not listen to socket!" << std::endl;
            // }
            std::cerr << "Could not listen to socket!" << std::endl;
            return;
        }

        this->state = ClientConnection::READY;
    }

     ~TCPHostConnection() {
        close(sockfd);
    }

    inline std::unique_ptr<ClientConnection> acceptConnection() override {
        struct sockaddr_in address;
        size_t addrlen = sizeof(address);
        int new_socket = accept(sockfd, (struct sockaddr*) &address, (socklen_t*) &addrlen);
        if (new_socket < 0) {
            if (this->blockmode/* || (!this->blockmode && errno != EWOULDBLOCK)*/) {
                this->state = ERROR;
                std::cerr << "Could not accept to socket!" << std::endl;
            }
            return nullptr;
        }
        std::string addr(inet_ntoa(address.sin_addr));
        auto ptr = std::make_unique<TCPClientConnection>(type, addr, sourcePort, ntohs(address.sin_port), true, true, new_socket, *(struct sockaddr*) &address);
        this->state = CONNECTED;
        return ptr;
    }

    inline virtual bool setBlocking(bool blockmode) {
        return blockmode == this->blockmode || sock::set_blocking(this->sockfd, blockmode);
    }

    inline void print(std::ostream& stream) const override {
        stream << "Server " << type << ": INADDR_ANY" << ':' << sourcePort;
    }

protected:
    int sockfd;
    struct sockaddr sock_addr;
};

class TCPClientConnection::Factory : public ClientConnection::Factory {
public:
    explicit Factory(ConnectionType type) : ClientConnection::Factory(type) {}

    static inline Factory from(ConnectionType type) {
        return Factory(type);
    }
    static inline Factory from(NetType n_type) {
        return from({TransportType::TCP, n_type});
    }

    inline std::unique_ptr<ClientConnection> create() const override {
        return std::make_unique<TCPClientConnection>(type, address, sourcePort, destinationPort, blockmode, reusemode);
    }
};

class TCPHostConnection::Factory : public HostConnection::Factory {
public:
    explicit Factory(ConnectionType type) : HostConnection::Factory(type) {}

    static inline Factory from(ConnectionType type) {
        return Factory(type);
    }
    static inline Factory from(NetType n_type) {
        return from({TransportType::TCP, n_type});
    }

    inline std::unique_ptr<HostConnection> create() const override {
        return std::make_unique<TCPHostConnection>(type, sourcePort, blockmode, reusemode);
    }
};
#endif