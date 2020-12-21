#ifndef SWARMTORRENT_CONNECTION_TCP_GENERIC_H
#define SWARMTORRENT_CONNECTION_TCP_GENERIC_H

#include <cstdint>
#include <sys/socket.h>
#include <iostream>
#include <cerrno>
namespace tcp {
    // Sends a generic tcp message with given length and flags
    inline bool sendmsg(int sockfd, const uint8_t* const msg, unsigned length, int flags) {
        return send(sockfd, msg, length, flags) >= 0;
    }

    // Receive a generic tcp message with given length and flags
    inline bool recvmsg(int sockfd, uint8_t* const msg, unsigned length, int flags) {
        return recv(sockfd, msg, length, flags) >= 0;
    }

    // Peeks length bytes from a received generic tcp message with given flags
    inline bool peekmsg(int sockfd, uint8_t* const msg, unsigned length, int flags) {
        bool x = recv(sockfd, msg, length, MSG_PEEK | flags) >= 0;
        if (!x) {
            std::cerr << "Experienced error" << strerror(errno) << "(" << errno << ")\n";
        }
        return x;
    }
}
#endif 
