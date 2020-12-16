#ifndef SHARED_UTIL_ANTICOLLISION_H
#define SHARED_UTIL_ANTICOLLISION_H

#include <cstdint>

namespace anticollision {
    // Boost library hash combiner
    inline void boost(size_t& a, const size_t& b) {
        a ^= b + 0x9e3779b9 + (a<<6) + (a>>2);
    }
}
#endif