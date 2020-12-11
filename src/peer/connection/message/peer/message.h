#ifndef PEERMESSAGE_H
#define PEERMESSAGE_H

#include <cstdlib>
#include <memory>

#include "shared/connection/message/message.h"

namespace message::peer {
    static const inline uint8_t id = 33;

    enum Tag : uint8_t {
        TEST = 0,
        EXCHANGE_REQ = 1,   // Request to start sharing data (other side responds with either OK or REJ)
        EXCHANGE_CLOSE = 2, // Close a request. The receiving side does not respond with anything.
        DATA_REQ = 3,       // Request a piece of data (other side responds with OK and a piece of data)
        DATA_REPLY = 4      // Reply containing a batch of data. first 8 bytes of body are fragment id, the rest is the data.
    };
    struct Header {
        size_t size;
        uint8_t formatType = id;
        Tag tag;
    };

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
        h.size = datasize+sizeof(message::peer::Header);
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
}
#endif