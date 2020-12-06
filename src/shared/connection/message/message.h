#ifndef CONNECTION_FORMAT_STANDARD_H
#define CONNECTION_FORMAT_STANDARD_H

#include <cstdlib>

#include "shared/connection/connection.h"

namespace message::standard {
    enum Tag : uint8_t {
        OK = 0,
        REJECT = 1,
        ERROR = 2
    };

    struct Header {
        size_t size;
        uint8_t formatType;
    };

    inline Header from(size_t datasize, Tag t) {
        Header h; 
        h.size = datasize + sizeof(Header);
        h.formatType = t;
        return h;
    }

    inline Header from_r(size_t size, Tag t) {
        Header h;
        h.size = size;
        h.formatType = t;
        return h;
    }

    inline bool recv(const std::unique_ptr<ClientConnection>& conn, Header& h) {
        return conn->peekmsg((uint8_t*) &h, sizeof(Header));
    }

    inline bool send(const std::unique_ptr<ClientConnection>& conn, Tag t) {
        Header h = {sizeof(Header), (uint8_t) t};
        return conn->sendmsg((uint8_t*) &h, sizeof(Header));
    }
}

#endif
