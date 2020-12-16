#ifndef FRAGMENT_H
#define FRAGMENT_H 

#include <cstdint>
#include <fstream>
#include <iostream>

#include "shared/torrent/metadata/metaData.h"

// Object responsible to read from and write to fragments for a single file
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
    FragmentHandler(uint64_t file_size, uint64_t fragment_size, uint64_t num_fragments, const std::string& path) : file_size(file_size), fragment_size(fragment_size), num_fragments(num_fragments), write_head(path, std::ios::out | std::ios::app | std::ios::binary), read_head(path, std::ios::in | std::ios::binary) {
        std::cerr << "Reading path '"<<path<<"' (file_size="<<file_size<< " bytes)\n";
        write_head.seekp(0, std::ios::beg);
        read_head.seekg(0, std::ios::beg);
    }
    FragmentHandler(const TorrentMetadata& meta, const std::string& path) : FragmentHandler(meta.size, meta.get_fragment_size(), meta.get_num_fragments(), path) {}
    ~FragmentHandler();

    // Reads fragment from file, saves it in data.
    // '''Note:''' data is allocated in this function. It requires to be freed at the end of its use.
    // '''Warning:''' It is up to the caller to find out whether file fragment is available or not
    // Returns false if read_head is invalid
    inline bool read(unsigned index, uint8_t*& data, unsigned& data_size) {
        return read_with_leading(index, data, data_size, 0);
    }

    // Just like [[read(index, data, data_size)]]. Allocates a buffer with leading_size extra bytes.
    // Reader allocates bytes after the leading bytes.
    // This function is useful if you want to add a header (e.g. for a message) without having to memcpy all read data.
    bool read_with_leading(unsigned index, uint8_t*& data, unsigned& data_size, unsigned leading_size);


    // Writes fragment to file
    // Returns whether the fragment is written to disk
    bool write(unsigned index, const uint8_t* data, unsigned data_size);
};

#endif