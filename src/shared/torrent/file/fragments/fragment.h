#ifndef FRAGMENT_H
#define FRAGMENT_H 

#include <fstream>

#include "../torrentFile.h"

class Fragment {
protected:
    std::ofstream write_head; // Current write_head position
    unsigned prev_write_index; // Previous Fragment index to which was written
    std::ifstream read_head; // Curent read_head position
    unsigned prev_read_index; // Previous Fragment index to which was read
    const TorrentFile torrentfile; // Torrentfile corresponding to Fragment

public:
    Fragment(const TorrentFile& torrentfile, const std::string& path) : write_head(path, std::ios::out | std::ios::binary), prev_write_index(0), read_head(path, std::ios::in | std::ios::binary), prev_read_index(0), torrentfile(torrentfile) {};
    ~Fragment();

    // Reads fragment from file, saves it in data
    // Warning: data requires to be freed at the end of its use
    // Pre: peer has fragment
    // Returns false if read_head is invalid
    bool read(unsigned index, uint8_t* data, unsigned& data_size);
    
    // Writes fragment to file, if the fragment is valid
    // Returns whether the fragment is valid
    bool write(unsigned index, const uint8_t* data, unsigned data_size);    
};

namespace fragment {

}

#endif