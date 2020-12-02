#ifndef CONNECTION_MESSAGE_INTERPRETABLE_H
#define CONNECTION_MESSAGE_INTERPRETABLE_H

#include <cstdint>

namespace message {
    struct Interpretable {
        /** Returns true if received message is valid for this message type, otherwise false */
        virtual bool valid() = 0;

        /** 
         * Interprets raw data as message.
         * @return true if received message is valid for this message type, otherwise false
         */
        virtual bool interpret(const uint8_t* const data) = 0;
    };
}
#endif