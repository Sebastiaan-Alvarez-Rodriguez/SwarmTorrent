#ifndef CONNECTION_FORMAT_STANDARD_H
#define CONNECTION_FORMAT_STANDARD_H

#include <cstdlib>

#include "shared/connection/connection.h"

namespace message::standard {
    static const inline uint8_t id = 0;
    enum Tag : uint8_t {
        OK = 0,
        REJECT = 1,
        ERROR = 2,
        LOCAL_DISCOVERY_REQ = 3,
        LOCAL_DISCOVERY_REPLY = 4
    };

    struct Header {
        size_t size;
        uint8_t formatType = id;
        Tag tag;
    };

    inline Header from(size_t datasize, Tag t) {
        Header h; 
        h.size = datasize + sizeof(Header);
        h.formatType = id;
        h.tag = t;
        return h;
    }

    inline Header from_r(size_t size, Tag t) {
        Header h;
        h.size = size;
        h.formatType = id;
        h.tag = t;
        return h;
    }

    inline bool recv(const std::unique_ptr<ClientConnection>& conn, Header& h) {
        return conn->peekmsg((uint8_t*) &h, sizeof(Header));
    }

    inline bool send(const std::unique_ptr<ClientConnection>& conn, Tag t) {
        Header h = {sizeof(Header), id, t};
        return conn->sendmsg((uint8_t*) &h, sizeof(Header));
    }
}

#endif
