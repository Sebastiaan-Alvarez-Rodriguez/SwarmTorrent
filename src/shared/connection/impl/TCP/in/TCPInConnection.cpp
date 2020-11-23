#include <iostream>
#include <cstring>

#include "shared/util/socket.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "TCPInConnection.h"


TCPInConnection::TCPInConnection(ConnectionType type, uint16_t port) : TCPConnection::TCPConnection(type), port(port) {
    this->state = Connection::ERROR;
    if ((sockfd = sock::make(type)) < 0) {
        std::cerr << "Could not build socket!" << std::endl;
        return;
    }
    
    struct sockaddr_in sock;
    std::memset((uint8_t*)&sock, 0, sizeof(sock));
    // bzero((char *) &sock, sizeof(sock));
    sock.sin_family = this->type.n_type.to_ctype();
    sock.sin_addr.s_addr = INADDR_ANY;
    sock.sin_port = htons(port);

    if(bind(sockfd, (const struct sockaddr*) &(sock), sizeof(sock)) < 0) {
        std::cerr << "Could not bind socket! Maybe address already in use?" << std::endl;
        return;
    }
    this->state = Connection::READY;
}