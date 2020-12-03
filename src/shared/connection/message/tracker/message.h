#ifndef TRACKERMESSAGE_H
#define TRACKERMESSAGE_H

#include <cstdlib>
#include <memory>

#include "shared/connection/message/message.h"
#include "shared/connection/message/interpretable.h"

namespace message::tracker {
    static const inline uint8_t id = 32;

    enum Tag : uint8_t {
        SUBSCRIBE = 0,
        UNSUBSCRIBE = 1,
        RECEIVE = 2,
        UPDATE = 4, 
        MAKE_TORRENT = 8
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
        h.size = datasize+sizeof(message::tracker::Header);
        h.formatType = id;
        h.tag = t;
        return h;
    }
}
#endif