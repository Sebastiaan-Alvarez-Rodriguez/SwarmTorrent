#ifndef SHARED_UTIL_VECTOR_H
#define SHARED_UTIL_VECTOR_H

#include <algorithm>
#include <vector>

namespace vec {
    void quick_or(std::vector<bool>& a, const std::vector<bool>& b) {
        std::vector<bool> v3(v1.size());
        std::transform(a.begin(), a.end(), b.begin(), b.begin(), std::logical_and<bool>());
    }
}
#endif