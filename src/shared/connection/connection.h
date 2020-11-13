#ifndef CONNECTION_H
#define CONNECTION_H

// Simple tutorial to get started
// https://www.geeksforgeeks.org/socket-programming-cc/
// https://www.tutorialspoint.com/cplusplus/cpp_interfaces.htm
#include "shared/connection/meta/state.h"

class Connection {
public:
    Connection();
    ~Connection();
    
    /** Returns true if connection succes, otherwise false */
    virtual bool connect() = 0;

    /** Returns type of connection we make */
    virtual ConnectionType type() = 0;
    /** Returns current state of the connection */
    virtual ConnectionState state() = 0;
};
#endif