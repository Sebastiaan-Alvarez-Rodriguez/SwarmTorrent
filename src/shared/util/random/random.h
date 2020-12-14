#ifndef SHARED_UTIL_RANDOM_H
#define SHARED_UTIL_RANDOM_H

#include <algorithm>

#include "randomGenerator.h"

namespace rnd {
    // Returns a random index of vector with value bool
    // Returns vector.size() if it does not exist
    inline size_t random_from(RandomGenerator<size_t>& rg, const std::vector<bool>& vec, bool value) {
        if (std::find(vec.begin(), vec.end(), value) == vec.end())
            return vec.size();

        while (true) {
            size_t index = rg.generate(0, vec.size()-1);
            if (vec[index] == value)
                return index;
        }
    }
}
#endif