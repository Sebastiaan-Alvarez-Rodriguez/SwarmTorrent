#ifndef UTIL_SOCKET_H
#define UTIL_SOCKET_H

#include <fcntl.h>
#include<sys/socket.h>

#include "shared/connection/meta/type.h"

namespace sock {
    inline int make(const ConnectionType& type) {
        int sockfd;
        if ((sockfd = socket(type.n_type.to_ctype(), type.t_type.to_ctype(), 0)) < 0)
            return -1;
        return sockfd;
    }

    /** 
     * Sets given socket in blocking mode if `blocking` is `false`, otherwise sets socket in `non-blocking` mode.
     * When non-blocking, operations like `send(), recv(), connect(), accept()` immediately return.
     *
     * For e.g. a `recv`, if there is no data available, `recv` will return -1 and set `errno` to `EWOULDBLOCK`. 
     * @return `true` on success, `false` otherwise
     */
    inline bool set_blocking(int fd, bool blocking) {
        if (fd < 0)
            return false;

        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) 
            return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(fd, F_SETFL, flags) == 0);
    }

    /**
     * Allows this socket to bind to sockets in internal TIME_WAIT state.
     * 
     * '''Note:''' Call this function before calling bind() on socket, because this setting is used then
     */
    inline bool set_reuse(int fd) {
        int opt = 1;
        return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(opt)) >= 0;
    }

    /**
     * Sets a send timeout for given socket.
     *
     * '''Note:''' Sockets that are already non-blocking may not behave well.
     * '''Warning:''' Not all socket types allow setting this option
     */
    inline bool set_timeout_send(int fd, int s, int ms) {
        struct timeval timeout;
        timeout.tv_sec = s;
        timeout.tv_usec = ms;
        return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*) &timeout, sizeof(timeout)) >= 0;
    }


    /**
     * Sets a recv timeout for given socket.
     *
     * '''Note:''' Sockets that are already non-blocking may not behave well.
     * '''Warning:''' Not all socket types allow setting this option
     */
    inline bool set_timeout_recv(int fd, int s, int ms) {
        struct timeval timeout;
        timeout.tv_sec = s;
        timeout.tv_usec = ms;
        return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout)) >= 0;
    }
}
#endif