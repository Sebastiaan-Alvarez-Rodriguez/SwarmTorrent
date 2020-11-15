#include <stdexcept>
#include <fstream>

#include "fileInfo.h"

static HashTable create_hash_table(std::string& path, unsigned fragment_size) {
    unsigned f_size = file_size(path);
    unsigned nr_fragments = ((f_size -1) / fragment_size) + 1;
    HashTable hashTable;
    
    for (unsigned i = 0; i < nr_fragments; ++i) {
        unsigned size = (i != nr_fragments-1) ? fragment_size : (f_size % fragment_size);
        const uint8_t* data = new uint8_t[size];
        std::ifstream f;
        f.open(path, std::ios::binary);
        f.read((char*)data, size);
        f.close();
        std::string hash;
        hash_sha256(hash, data, size);
        delete data;
        if (!hashTable.add_hash(hash))
            throw std::runtime_error("Creating HashTable failed");
    }
    return hashTable;
}

FileInfo::FileInfo(std::string path, unsigned fragment_size, unsigned divide_threshold, TrackerTable trackertable) : torrentfile(trackertable, basename(path), file_size(path), fragment_size, (((file_size(path) -1) / fragment_size) + 1) / divide_threshold, create_hash_table(path, fragment_size)), path(path) {
    unsigned size = file_size(path);
    unsigned nr_fragments = ((size -1) / fragment_size) + 1;
    received.assign(nr_fragments, true);
    this->nr_fragments = nr_fragments;
}

