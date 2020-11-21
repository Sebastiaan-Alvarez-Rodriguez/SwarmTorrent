#ifndef UTIL_PRINT_H
#define UTIL_PRINT_H

#include <iostream>
#include <string>

namespace print {
    enum color {
        CLEAR = 0,
        CYAN = 36, 
        GREEN = 32,
        RED = 31,    
        YELLOW = 33, 
        WHITE = 37
    };

    inline std::ostream& operator<<(std::ostream& stream, color val) {
        return stream << "\033[" << static_cast<int>(val) << "m";
    }
}
#endif