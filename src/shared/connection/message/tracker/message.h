#ifndef TRACKERMESSAGE_H
#define TRACKERMESSAGE_H

#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <vector>

#include "shared/connection/message/message.h"

namespace message::tracker {
    static const inline uint8_t id = 32;

    // Supported Tags for this message type
    enum Tag : uint8_t {
        TEST = 1,
        MAKE_TORRENT = 2,
        REGISTER = 4,
        RECEIVE = 8,
        UPDATE = 16
    };

    struct Header {
        size_t size; // Size of the data
        uint8_t formatType = id; // formatType id for this message type
        Tag tag; // Tag for this message

        // Write header to byte array
        inline void write(uint8_t* const ptr) const {
            uint8_t* writer = ptr;
            *(size_t*) writer = size;
            writer += sizeof(size_t);

            *writer = formatType;
            writer += sizeof(uint8_t);

            *writer = (uint8_t) tag;
        }

        // Read header from byte array 
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

    // Returns the size in bytes of a header
    constexpr inline static size_t bytesize() {
        return sizeof(size_t)+sizeof(uint8_t)+sizeof(Tag);
    }

    // Writes a given header to a byte array
    inline void write(const Header& h, uint8_t* const ptr) {
        h.write(ptr);
    }

    // Initializes a Header 
    inline Header from(Tag t) {
        Header h;
        h.formatType = id;
        h.tag = t;
        return h;
    }
    
    // Initializes a Header.
    // '''Note:''' provided size is assumed to be size of extra data. Header size is added.
    inline Header from(size_t datasize, Tag t) {
        Header h;
        h.size = datasize+bytesize();
        h.formatType = id;
        h.tag = t;
        return h;
    }

    // Returns the header from a connection by peeking the received message
    inline Header recv(const std::unique_ptr<ClientConnection>& conn) {
        uint8_t data[bytesize()];
        conn->peekmsg(data, bytesize());
        return Header::read(data);
    }
}
#endif