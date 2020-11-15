#include <sstream>

#include "hasher.h"

void hash_sha256(uint8_t hash[32], const uint8_t* data, unsigned size) {
    sha256_ctx sha;
    sha256_init(&sha);
    sha256_chunk(&sha, data, size);
    sha256_final(&sha);
    sha256_hash(&sha, hash);
}

void hash_sha256(std::string& hash, const uint8_t* data, unsigned size) {
    uint8_t arr[32]; 
    hash_sha256(arr, data, size);
    std::ostringstream converter;
    for (unsigned i = 0; i < 32; ++i)
        converter << (char)arr[i];
    hash = converter.str();
}