#ifndef FRAGMENT_H
#define FRAGMENT_H 

namespace fragment {
    // Reads fragment from file, saves it in data
    // Warning: data requires to be freed at the end of its use
    // Returns false if peer does not have fragment
    bool read(unsigned index, uint8_t* data, unsigned& data_size);
    
    // Writes fragment to file, if the fragment is valid
    // Returns whether the fragment is valid
    bool write(unsigned index, const uint8_t* data, unsigned data_size);
}

#endif