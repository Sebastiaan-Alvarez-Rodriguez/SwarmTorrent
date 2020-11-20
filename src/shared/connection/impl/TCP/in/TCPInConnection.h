#ifndef TCPINCONNECTION_H
#define TCPINCONNECTION_H

#include <cstdint>
#include <string>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "shared/connection/connection.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/meta/type.h"

class TCPInConnection : public TCPConnection {
public:
    class Factory;
    TCPInConnection(ConnectionType type, uint16_t port);
    ~TCPInConnection() { close(sockfd);}

    inline void print(std::ostream& stream) const override {
        stream << "Inbound " << type << ": " << port;
    }

protected:
    uint16_t port;

    int sockfd;
};


class TCPInConnection::Factory : Connection::Factory {
public:
    Factory(ConnectionType type) : Connection::Factory::Factory(type) {}

    static inline Factory from(ConnectionType type) {
        return Factory(type);
    }
    static inline Factory from(NetType n_type) {
        return from({TransportType::TCP, n_type});
    }

    Factory& withPort(uint16_t p) {
        port = p;
        return *this;
    }

    std::unique_ptr<Connection> create() const override {
        return std::make_unique<TCPInConnection>(type, port);
    }

protected:
    uint16_t port = 0;
};
#endif