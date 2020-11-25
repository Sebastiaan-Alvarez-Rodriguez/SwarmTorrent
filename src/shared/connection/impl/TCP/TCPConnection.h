#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <cstdint>
#include <string>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "shared/connection/meta/type.h"
#include "shared/connection/connection.h"

class TCPConnection : public Connection {
public:
    TCPConnection(ConnectionType type) : Connection::Connection(type) {}

    ~TCPConnection() {
        close(sockfd);
    }

    inline void sendmsg(const void* const msg, unsigned length) const override { send(sockfd, msg, length, 0); };
    inline void recvmsg(void* const msg, unsigned length) const override { read(sockfd, msg, length); };

    // inline virtual void print(std::ostream& stream) const {
    //     stream << type << ": " << address << ':' << port;
    // }
protected:
    int sockfd;
    struct sockaddr sock;
};
#endif