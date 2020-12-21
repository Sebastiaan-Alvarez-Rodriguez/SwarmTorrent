#ifndef UTIL_PRINT_H
#define UTIL_PRINT_H

#include <iostream>
#include <string>

namespace print {
    // POSIX color codes
    enum color {
        CLEAR = 0,
        CYAN = 36, 
        GREEN = 32,
        RED = 31,    
        YELLOW = 33, 
        WHITE = 37
    };

    // Returns given stream with given color
    inline std::ostream& operator<<(std::ostream& stream, color val) {
        return stream << "\033[" << static_cast<int>(val) << "m";
    }
}
#endif