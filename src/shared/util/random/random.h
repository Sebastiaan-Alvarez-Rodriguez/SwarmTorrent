#include <algorithm>

#include "randomGenerator.h"

namespace random {
    // Returns a random index of vector with value bool
    // Returns vector.size() if it does not exist
    size_t randomIndex(RandomGenerator& rg, const std::vector<bool>& vec, bool value) {
        if (std::find(vec.begin(), vec.end(), value) == vec.end())
            return vec.size();

        while (true) {
            size_t index = rg.generate(0, vec.size()-1);
            if (vec[index] == value)
                return index;
        }
    }

}