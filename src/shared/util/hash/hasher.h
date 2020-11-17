#ifndef HASHER_H
#define HASHER_H

#include <string>
#include "sha256.h"

namespace hash {
    enum Hash {
        SHA256_H = 32
    };

    // Computes the sha256 hash of the given data
    // Returns a byte array
    void sha256(uint8_t hash[32], const uint8_t* data, unsigned size);
    // Computes the sha256 hash of the given data
    // Returns a string
    void sha256(std::string& hash, const uint8_t* data, unsigned size);
}
#endif