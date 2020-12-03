#ifndef CONNECTION_FORMAT_STANDARD_H
#define CONNECTION_FORMAT_STANDARD_H

#include <cstdlib>

#include "shared/connection/connection.h"

namespace message::standard {
    enum type : uint8_t {
        OK = 0,
        ERROR = 1
    };

    struct Header {
        size_t size;
        uint8_t formatType;
    };

    inline Header make(size_t datasize, type t) {
        Header h; 
        h.size = datasize + sizeof(Header);
        h.formatType = t;
        return h;
    }

    inline bool from(const std::unique_ptr<ClientConnection>& conn, Header& h) {
        return conn->peekmsg((uint8_t*) &h, sizeof(Header));
    }
    inline bool send(const std::unique_ptr<ClientConnection>& conn, type t) {
        Header h = {sizeof(Header), (uint8_t) t};
        return conn->sendmsg((uint8_t*) &h, sizeof(Header));
    }
}

#endif
