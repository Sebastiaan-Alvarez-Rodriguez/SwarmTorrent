#ifndef CONNECTION_H
#define CONNECTION_H

// Simple tutorial to get started
// https://www.geeksforgeeks.org/socket-programming-cc/

#include "shared/connection/meta/state.h"

class Connection {
public:
    Connection();
    ~Connection();
    
    /** Returns true if connection succes, otherwise false */
    bool connect();

    /** Returns type of connection we make */
    ConnectionType type();
    /** Returns current state of the connection */
    ConnectionState state();
};
#endif