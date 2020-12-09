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
    Connection(ConnectionType type, uint16_t sourcePort) : type(type), sourcePort(sourcePort) {};
    ~Connection() = default;

    /** Returns type of connection we use */
    virtual const ConnectionType& get_type() {return type;}

    inline virtual void print(std::ostream& stream) const = 0;

    inline virtual uint16_t getSourcePort() const {return sourcePort;}


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

    uint16_t sourcePort;
};

class ClientConnection : public Connection {
public:
    explicit ClientConnection(ConnectionType type, std::string address, uint16_t sourcePort, uint16_t destinationPort) : Connection(type, sourcePort), address(std::move(address)), destinationPort(destinationPort) {};
    ~ClientConnection() = default;

    class Factory;

    inline virtual const std::string& getAddress() const {
        return address;
    }

    inline virtual uint16_t getDestinationPort() const {
        return destinationPort;
    }

    virtual bool doConnect() = 0;
    virtual bool sendmsg(const uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool sendmsg(const uint8_t* const msg, unsigned length) const { return sendmsg(msg, length, 0); }

    virtual bool recvmsg(uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool recvmsg(uint8_t* const msg, unsigned length) const { return recvmsg(msg, length, MSG_WAITALL); }

    virtual bool peekmsg(uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool peekmsg(uint8_t* const msg, unsigned length) const { return peekmsg(msg, length, MSG_WAITALL); }

    virtual bool discardmsg(unsigned length) const = 0;
protected:
    std::string address;
    uint16_t destinationPort;

};

class HostConnection : public Connection {
public:
    explicit HostConnection(ConnectionType type, uint16_t hostPort) : Connection(type, hostPort) {};
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

    /**
     * Sets our local port to use for the connection.
     *
     * '''Convention:''' If `port=0` given (default), we use a random source port
     * @param p Source port to use
     * @return current factory instance
     */
    Factory& withSourcePort(uint16_t p) {
        sourcePort = p;
        return *this;
    }

    Factory& withDestinationPort(uint16_t p) {
        destinationPort = p;
        return *this;
    }

    virtual std::unique_ptr<ClientConnection> create() const = 0;

protected:
    ConnectionType type;
    std::string address;
    uint16_t sourcePort = 0, destinationPort = 0;
};

class HostConnection::Factory {
public:
    explicit Factory(ConnectionType type) : type(type) {}

    /**
    * Sets our local port to use for listening for connections.
    *
    * '''Convention:''' If `port=0` given (default), we use a random source port
    * @param p Source port to use
    * @return current factory instance
    */
    Factory& withSourcePort(uint16_t p) {
        sourcePort = p;
        return *this;
    }

    virtual std::unique_ptr<HostConnection> create() const = 0;

protected:
    ConnectionType type;
    uint16_t sourcePort = 0;
};
#endif