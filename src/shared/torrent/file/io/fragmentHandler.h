#ifndef FRAGMENT_H
#define FRAGMENT_H 

#include <cstdint>
#include <fstream>

#include "shared/torrent/metadata/metaData.h"

class FragmentHandler {
protected:
    const uint64_t file_size;
    const uint64_t fragment_size;
    const uint64_t num_fragments;

    std::ofstream write_head; // Current write_head position
    unsigned prev_write_index = 0; // Previous Fragment index to which was written
    std::ifstream read_head; // Curent read_head position
    unsigned prev_read_index = 0; // Previous Fragment index to which was read

public:
    FragmentHandler(uint64_t file_size, uint64_t fragment_size, uint64_t num_fragments, const std::string& path) : file_size(file_size), fragment_size(fragment_size), num_fragments(num_fragments), write_head(path, std::ios::out | std::ios::binary), read_head(path, std::ios::in | std::ios::binary) {};
    FragmentHandler(const TorrentMetadata& meta, const std::string& path) : FragmentHandler(meta.size, meta.get_fragment_size(), meta.get_num_fragments(), path) {}
    ~FragmentHandler();

    // Reads fragment from file, saves it in data
    // Warning: data requires to be freed at the end of its use
    // Pre: peer has fragment
    // Returns false if read_head is invalid
    bool read(unsigned index, uint8_t* data, unsigned& data_size);
    
    // Writes fragment to file, if the fragment is valid
    // Returns whether the fragment is valid
    bool write(unsigned index, const uint8_t* data, unsigned data_size);    
};

#endif