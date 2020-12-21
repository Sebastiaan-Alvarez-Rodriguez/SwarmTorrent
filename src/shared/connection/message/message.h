#ifndef CONNECTION_FORMAT_STANDARD_H
#define CONNECTION_FORMAT_STANDARD_H

#include <cstdlib>
#include <memory>

#include "shared/connection/connection.h"

namespace message::standard {
    static const inline uint8_t id = 0;

    // Enum for supported messages Tag
    enum Tag : uint8_t {
        OK = 0,
        REJECT = 1,
        ERROR = 2,
        LOCAL_DISCOVERY_REQ = 3,
        LOCAL_DISCOVERY_REPLY = 4
    };

    // Header to use for messages
    struct Header {
        size_t size; // Size of the message
        uint8_t formatType = id; // formattype of the message 
        Tag tag; // Tag of the message

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

    // Constructs a header with given datasize and tag
    // Headersize will be added
    inline Header from(size_t datasize, Tag t) {
        Header h; 
        h.size = datasize + bytesize();
        h.formatType = id;
        h.tag = t;
        return h;
    }

    // Constructs a header with given size and tag
    // Size should include headersize
    inline Header from_r(size_t size, Tag t) {
        Header h;
        h.size = size;
        h.formatType = id;
        h.tag = t;
        return h;
    }

    // Returns the header from a connection by peeking the received message
    inline Header recv(const std::shared_ptr<ClientConnection>& conn) {
        uint8_t data[bytesize()];
        conn->peekmsg(data, bytesize());
        return Header::read(data);
    }

    // Returns the header from a connection by peeking the received message
    inline Header recv(const std::unique_ptr<ClientConnection>& conn) {
        uint8_t data[bytesize()];
        conn->peekmsg(data, bytesize());
        return Header::read(data);
    }

    // Sends a header with given tag over the connection
    inline bool send(const std::shared_ptr<ClientConnection>& conn, Tag t) {
        Header h = {bytesize(), id, t};
        return conn->sendmsg((uint8_t*) &h, bytesize());
    }

    // Sends a header with given tag over the connection
    inline bool send(const std::unique_ptr<ClientConnection>& conn, Tag t) {
        Header h = {bytesize(), id, t};
        return conn->sendmsg((uint8_t*) &h, bytesize());
    }
}

#endif
