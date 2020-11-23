#ifndef TCPOUTCONNECTION_H
#define TCPOUTCONNECTION_H

#include <cstdint>
#include <string>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "shared/connection/connection.h"
#include "shared/connection/meta/type.h"
#include "shared/connection/impl/TCP/TCPConnection.h"


class TCPOutConnection : public TCPConnection {
public:
    class Factory;
    TCPOutConnection(ConnectionType type, std::string address, uint16_t port);
    ~TCPOutConnection() {close(sockfd);}
    
    inline void print(std::ostream& stream) const override {
        stream << "Outbound " << type << ": " << address << ':' << port;
    }
protected:
    std::string address;
    uint16_t port;

    int sockfd;
    struct sockaddr sock;
};


class TCPOutConnection::Factory : Connection::Factory {
public:
    Factory(ConnectionType type) : Connection::Factory::Factory(type) {}

    static inline Factory from(ConnectionType type) {
        return Factory(type);
    }
    static inline Factory from(NetType n_type) {
        return from({TransportType::TCP, n_type});
    }

    Factory& withAddress(std::string addr) {
        address = addr;
        return *this;
    }

    Factory& withPort(uint16_t p) {
        port = p;
        return *this;
    }

    std::unique_ptr<Connection> create() const override {
        return std::make_unique<TCPOutConnection>(type, address, port);
    }

protected:
    std::string address = "";
    uint16_t port = 0;
};
#endif