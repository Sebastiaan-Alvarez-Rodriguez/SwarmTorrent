#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <cstdint>
#include <string>

#include "shared/connection/meta/type.h"
#include "shared/connection/connection.h"

class TCPConnection : public Connection {
public:
    TCPConnection(ConnectionType type, std::string address, uint16_t port) : Connection::Connection(type), address(address), port(port) {};
    ~TCPConnection() = default;
    
    /** Returns true if connection succes, otherwise false */
    bool connect() {
        return false;
    }

    /** Returns type of connection we use */
    inline virtual const ConnectionType& get_type() {return type;}

    inline void print(std::ostream& stream) const override {
        stream << type << ": " << address << ':' << port;
    }
protected:
    std::string address;
    uint16_t port;
};
#endif