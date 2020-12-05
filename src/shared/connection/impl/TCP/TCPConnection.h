#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

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
// Need to explicitly bind to port for client connections?
class TCPClientConnection : public ClientConnection {
public:
    class Factory;

    explicit inline TCPClientConnection(ConnectionType type, const std::string& address, uint16_t port) : ClientConnection::ClientConnection(type, address, port) {
        this->state = ClientConnection::ERROR;
        if ((this->sockfd = sock::make(type)) < 0) {
            std::cerr << "Could not build socket!" << std::endl;
            return;
        }

        struct sockaddr_in addr_in;
        addr_in.sin_family = type.n_type.to_ctype();
        addr_in.sin_port = htons(port);

        if(inet_pton(type.n_type.to_ctype(), address.c_str(), &addr_in.sin_addr) <= 0) {
            std::cerr << "Could not assign address!" << std::endl;
            return;
        }
        this->sock_addr = *(struct sockaddr*) &addr_in;
        this->state = READY;
    }

    explicit inline TCPClientConnection(ConnectionType type, const std::string& address, uint16_t port, int sockfd, struct sockaddr sock_addr): ClientConnection::ClientConnection(type, address, port), sockfd(sockfd), sock_addr(sock_addr) {
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

    inline virtual bool discardmsg(unsigned length) const {
        // uint8_t* buf = malloc(length);
        return recvmsg(NULL, 0, MSG_TRUNC);
    };

    inline void print(std::ostream& stream) const override {
        stream << "Client target:" << type << ": " << address << ':' << port;
    }

protected:
    int sockfd;
    struct sockaddr sock_addr;
};

class TCPHostConnection: public HostConnection {
public:
    class Factory;

    explicit inline TCPHostConnection(ConnectionType type, uint16_t port) : HostConnection(type, port) {
        this->state = ClientConnection::ERROR;
        if ((sockfd = sock::make(type)) < 0) {
            std::cerr << "Could not build socket!" << std::endl;
            return;
        }

        struct sockaddr_in* addr_in = (struct sockaddr_in*) &(this->sock_addr);
        std::memset((uint8_t*)addr_in, 0, sizeof(sockaddr_in));
        // bzero((char *) &sock, sizeof(sock));
        addr_in->sin_family = this->type.n_type.to_ctype();
        addr_in->sin_addr.s_addr = INADDR_ANY;
        addr_in->sin_port = htons(port);

        if(bind(sockfd, (struct sockaddr*) addr_in, sizeof(sockaddr_in)) < 0) {
            std::cerr << "Could not bind socket! Maybe address already in use?" << std::endl;
            return;
        }
        this->state = ClientConnection::READY;
    }

     ~TCPHostConnection() {
        close(sockfd);
    }

    inline std::unique_ptr<ClientConnection> acceptConnection() override {
        if (listen(sockfd, 10) < 0) { //TODO: Change hardcoded 10 for connection backlog to a global constant somewhere
            std::cerr << "Could not listen to socket!" << std::endl;
            return nullptr;
        }

        struct sockaddr_in address;
        size_t addrlen = sizeof(address);
        int new_socket = accept(sockfd, (struct sockaddr*) &address, (socklen_t*) &addrlen);
        if (new_socket < 0) {
            std::cerr << "Could not accept to socket!" << std::endl;
            return nullptr;
        }
        std::string addr(inet_ntoa(address.sin_addr));
        auto ptr = std::make_unique<TCPClientConnection>(type, addr, ntohs(address.sin_port), new_socket, *(struct sockaddr*) &address);
        this->state = CONNECTED;
        return ptr;
    }

    inline void print(std::ostream& stream) const override {
        stream << "Server " << type << ": INADDR_ANY" << ':' << port;
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
        return std::make_unique<TCPClientConnection>(type, address, port);
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
        return std::make_unique<TCPHostConnection>(type, port);
    }
};
#endif