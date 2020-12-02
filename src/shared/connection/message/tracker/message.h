#ifndef TRACKERMESSAGE_H
#define TRACKERMESSAGE_H

#include <cstdlib>
#include <memory>

#include "shared/connection/message/message.h"
#include "shared/connection/message/interpretable.h"

namespace message {
    struct TrackerMessage : Interpretable {
        static const inline uint8_t id = 1;

        enum Tag : uint8_t {
            SUBSCRIBE = 0,
            UNSUBSCRIBE = 1,
            RECEIVE = 2,
            UPDATE = 4
        };
        struct Header {
            size_t size;
            uint8_t formatType;
            Tag tag;
        } header;



        /** Initializes TrackerMessage */
        inline void init(Tag t) {
            header.formatType = id;
            header.tag = t;
        }

        /** Constructs and returns TrackerMessage to send, with given type */
        inline static TrackerMessage fromType(Tag t) {
            TrackerMessage m;
            m.init(t);
            return m;
        }

        /** Sets header size. */
        virtual TrackerMessage& withSize(size_t size) {
            header.size = size;
            return *this;
        }

        /** 
         * Sets header size. 
         * '''Note:''' Only provide the size of data behind the header. Header size is added automatically.
         */
        virtual TrackerMessage& withDataSize(size_t size) {
            header.size = sizeof(Header)+size;
            return *this;
        }

        inline bool valid() {
            return header.formatType == id;
        }
        inline bool interpret(const uint8_t* const data) override {
            header.tag = (Tag) *(data);
            return true;
        }
    };
}
#endif