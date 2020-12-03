#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


// Simple tutorial to get started
// https://www.geeksforgeeks.org/socket-programming-cc/
// https://www.tutorialspoint.com/cplusplus/cpp_interfaces.htm
#include "shared/connection/meta/type.h"



class Connection {
public:
    Connection(ConnectionType type, uint16_t port) : type(type), port(port) {};
    ~Connection() = default;

    /** Returns type of connection we use */
    virtual const ConnectionType& get_type() {return type;}

    inline virtual void print(std::ostream& stream) const = 0;

    inline virtual uint16_t getPort() const {return port;}


    enum State {
        DISCONNECTED,
        READY,
        CONNECTED,
        ERROR
    };

    /** Returns current state of the connection */
    virtual const State& get_state() {return state;}

protected:
    State state = State::DISCONNECTED;
    const ConnectionType type;

    uint16_t port;
};

class ClientConnection : public Connection {
public:
    explicit ClientConnection(ConnectionType type, std::string address, uint16_t port) : Connection(type, port), address(std::move(address)) {};
    ~ClientConnection() = default;

    class Factory;

    inline virtual const std::string& getAddress() const {
        return address;
    }

    virtual bool doConnect() = 0;
    virtual bool sendmsg(const uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool sendmsg(const uint8_t* const msg, unsigned length) const { return sendmsg(msg, length, 0); }

    virtual bool recvmsg(uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool recvmsg(uint8_t* const msg, unsigned length) const { return recvmsg(msg, length, MSG_WAITALL); }

    virtual bool peekmsg(uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool peekmsg(uint8_t* const msg, unsigned length) const { return peekmsg(msg, length, MSG_WAITALL); }

protected:
    std::string address;

};

class HostConnection : public Connection {
public:
    explicit HostConnection(ConnectionType type, uint16_t port) : Connection(type, port) {};
    ~HostConnection() = default;

    class Factory;

    virtual std::unique_ptr<ClientConnection> acceptConnection() = 0;
};


inline std::ostream& operator<<(std::ostream& stream, const ClientConnection& connection) {
    connection.print(stream);
    return stream;
}


class ClientConnection::Factory {
public:
    explicit Factory(ConnectionType type) : type(type) {}

    Factory& withAddress(std::string addr) {
        address = std::move(addr);
        return *this;
    }

    Factory& withPort(uint16_t p) {
        port = p;
        return *this;
    }

    virtual std::unique_ptr<ClientConnection> create() const = 0;

protected:
    ConnectionType type;
    std::string address;
    uint16_t port = 0;
};

class HostConnection::Factory {
public:
    explicit Factory(ConnectionType type) : type(type) {}

    Factory& withPort(uint16_t p) {
        port = p;
        return *this;
    }

    virtual std::unique_ptr<HostConnection> create() const = 0;

protected:
    ConnectionType type;
    uint16_t port = 0;
};
#endif