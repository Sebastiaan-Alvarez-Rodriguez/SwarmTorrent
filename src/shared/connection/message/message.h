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

        inline void write(uint8_t* const ptr) const {
            uint8_t* writer = ptr;
            *(size_t*) writer = size;
            writer += sizeof(size_t);

            *writer = formatType;
            writer += sizeof(uint8_t);

            *writer = (uint8_t) tag;
        }

        inline static Header read(const uint8_t* const ptr) {
            const uint8_t* reader = ptr;

            Header h;
            h.size = *(size_t*) reader;
            reader += sizeof(size_t);

            h.formatType = *(uint8_t*) reader;
            reader += sizeof(uint8_t);

            h.tag = *(Tag*) reader;
            return h;
        }
    };

    constexpr inline static size_t bytesize() {
        return sizeof(size_t)+sizeof(uint8_t)+sizeof(Tag);
    }

    inline void write(const Header& h, uint8_t* const ptr) {
        h.write(ptr);
    }

    inline Header from(size_t datasize, Tag t) {
        Header h; 
        h.size = datasize + bytesize();
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

    inline Header recv(const std::unique_ptr<ClientConnection>& conn) {
        uint8_t data[bytesize()];
        conn->peekmsg(data, bytesize());
        return Header::read(data);
    }

    inline bool send(const std::unique_ptr<ClientConnection>& conn, Tag t) {
        Header h = {bytesize(), id, t};
        return conn->sendmsg((uint8_t*) &h, bytesize());
    }
}

#endif
