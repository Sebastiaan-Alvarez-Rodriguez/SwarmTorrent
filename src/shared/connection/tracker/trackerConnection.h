#ifndef TRACKERCONNECTION_H
#define TRACKERCONNECTION_H

#include <cstdlib>
#include <memory>

#include "shared/connection/impl/TCP/TCPConnection.h"

enum TAG {
    SUBSCRIBE = 0, 
    UNSUBSCRIBE, 
    RECEIVE, 
    UPDATE
};

enum STATUS {
    REQUEST = 0, 
    CONFIRM, 
    ERROR
};

struct Header {
    TAG tag;
    STATUS status;  
    size_t size;
};

void send_header(std::unique_ptr<Connection> connection, const Header& header);
void receive_header(std::unique_ptr<Connection> connection, Header& header);

#endif