#include "shared/util/socket.h"

#include "shared/connection/impl/TCP/TCPConnection.h"
#include "TCPOutConnection.h"


TCPOutConnection::TCPOutConnection(ConnectionType type, std::string address, uint16_t port) : TCPConnection::TCPConnection(type), address(address), port(port) {
    this->state = Connection::ERROR;
    if ((this->sockfd = sock::make(type)) < 0) {
        std::cerr << "Could not build socket!" << std::endl;
        return;
    }

    struct sockaddr_in* addr_in = (struct sockaddr_in*) &(this->sock);
    addr_in->sin_family = AF_INET;
    addr_in->sin_port = htons(port);

    if(inet_aton(address.c_str(), &addr_in->sin_addr) == 0) {
        std::cerr << "Could not assign address!" << std::endl;
        return;
    }
    this->state = Connection::READY;
}