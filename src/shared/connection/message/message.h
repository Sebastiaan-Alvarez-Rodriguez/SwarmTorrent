#ifndef CONNECTION_FORMAT_GENERIC_H
#define CONNECTION_FORMAT_GENERIC_H

#include <cstdlib>
#include <memory>

#include "shared/connection/message/interpretable.h"

namespace message {
    // All messages begin with a size, and a format identifier
    struct generic {
        size_t size;
        uint8_t formatType;
    };
}
#endif
