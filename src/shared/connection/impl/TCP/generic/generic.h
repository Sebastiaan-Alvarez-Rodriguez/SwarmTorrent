#ifndef SWARMTORRENT_CONNECTION_TCP_GENERIC_H
#define SWARMTORRENT_CONNECTION_TCP_GENERIC_H

#include <cstdint>
#include <sys/socket.h>
#include <iostream>
#include <cerrno>
namespace tcp {
    inline bool sendmsg(int sockfd, const uint8_t* const msg, unsigned length, int flags) {
        return send(sockfd, msg, length, flags) == 0;
    }

    inline bool recvmsg(int sockfd, uint8_t* const msg, unsigned length, int flags) {
        return recv(sockfd, msg, length, flags) == 0;
    }

    inline bool peekmsg(int sockfd, uint8_t* const msg, unsigned length, int flags) {
        bool x = recv(sockfd, msg, length, MSG_PEEK | flags) >= 0;
        if (!x) {
            std::cerr << "Experienced error" << strerror(errno) << "(" << errno << ")\n";
        }
        return x;
    }
}
#endif 
