#ifndef UTIL_SOCKET_H
#define UTIL_SOCKET_H

#include<sys/socket.h>

#include "shared/connection/meta/type.h"

namespace sock {
    inline int make(const ConnectionType& type) {
        int sockfd;
        if ((sockfd = socket(type.n_type.to_ctype(), type.t_type.to_ctype(), 0)) < 0)
            return -1;
        return sockfd;
    }
}
#endif