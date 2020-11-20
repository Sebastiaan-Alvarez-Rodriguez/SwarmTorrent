#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>


// Simple tutorial to get started
// https://www.geeksforgeeks.org/socket-programming-cc/
// https://www.tutorialspoint.com/cplusplus/cpp_interfaces.htm
#include "shared/connection/meta/state.h"
#include "shared/connection/meta/type.h"

class Connection {
public:
    Connection(ConnectionType type) : type(type) {};
    ~Connection() = default;
    class Factory;

    /** Returns true if connection succes, otherwise false */
    virtual bool connect() = 0;

    /** Returns type of connection we use */
    virtual const ConnectionType& get_type() = 0;
    /** Returns current state of the connection */
    virtual const ConnectionState& get_state() {return state;}


    inline virtual void print(std::ostream& stream) const = 0;
protected:
    const ConnectionType type;
    ConnectionState state = ConnectionState::DISCONNECTED;
};

inline std::ostream& operator<<(std::ostream& stream, const Connection& connection) {
    connection.print(stream);
    return stream;
}


class Connection::Factory {
public:
    Factory(ConnectionType type) : type(type) {}

    virtual std::unique_ptr<Connection> create() const = 0;

protected:
    ConnectionType type;
};
#endif