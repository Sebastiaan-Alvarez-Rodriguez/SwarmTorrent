#ifndef TCP_FACTORY_H
#define TCP_FACTORY_H

#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/connection.h"


class TCPFactory : Connection::Factory {
public:
    TCPFactory(ConnectionType type) : Connection::Factory(type) {}

    static inline TCPFactory from(ConnectionType type) {
        return TCPFactory(type);
    }
    static inline TCPFactory from(NetType n_type) {
        return from({TransportType::TCP, n_type});
    }


    TCPFactory& withAddress(std::string addr) {
        address = addr;
        return *this;
    }

    TCPFactory& withPort(uint16_t p) {
        port = p;
        return *this;
    }

    std::unique_ptr<Connection> create() const override {
        return std::make_unique<TCPConnection>(type, address, port);
    }

protected:
    std::string address = "";
    uint16_t port = 0;
};

#endif