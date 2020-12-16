#ifndef PEERMESSAGE_H
#define PEERMESSAGE_H

#include <cstdlib>
#include <memory>

#include "shared/connection/message/message.h"

namespace message::peer {
    static const inline uint8_t id = 33;

    enum Tag : uint8_t {
        TEST = 0,
        JOIN = 1,        // Request to start sharing data (other side responds with either OK or REJ).
        LEAVE = 2,       // Close a request. The receiving side does not respond with anything.
        DATA_REQ = 3,    // Request a piece of data (other side responds with OK and a piece of data).
        DATA_REPLY = 4,  // Reply containing a batch of data. first 8 bytes of body are fragment id, the rest is the data.
        INQUIRE = 5,     // Check if peer is dead.
        AVAILABILITY = 6 // Request the availability arrays
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


    /** Initializes a Header */
    inline Header from(Tag t) {
        Header h;
        h.formatType = id;
        h.tag = t;
        return h;
    }

    /**
     * Initializes a Header.
     * '''Note:''' provided size is assumed to be size of extra data. Header size will be added.
     */
    inline Header from(size_t datasize, Tag t) {
        Header h;
        h.size = datasize+bytesize();
        h.formatType = id;
        h.tag = t;
        return h;
    }

    /** Initializes a header */
    inline Header from_r(size_t size, Tag t) {
        Header h;
        h.size = size;
        h.formatType = id;
        h.tag = t;
        return h;
    }
}
#endif