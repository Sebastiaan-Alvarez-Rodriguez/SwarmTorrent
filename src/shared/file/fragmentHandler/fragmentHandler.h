#ifndef FRAGMENTHANDLER_H
#define FRAGMENTHANDLER_H 

#include "../fileInfo/fileInfo.h"

class FragmentHandler {
public:
    //Constructs the FragmentHandler with a FileInfo
    FragmentHandler(FileInfo fileinfo) : fileinfo(fileinfo) {};

    //Writes fragment to file, if the fragment is valid
    //Sets may_seed to true if number of received fragments exceeds a threshold
    //Returns whether the fragment is valid
    bool write_fragment(unsigned index, const uint8_t* data, unsigned data_size); 

    //Reads fragment from file, saves it in data
    //data requires to be freed at the end of its use
    //Returns false if peer does not have fragment
    bool read_fragment(unsigned index, uint8_t* data, unsigned& data_size);

    //Returns if peer is allowed to seed
    bool is_seeder() const {return may_seed;};

private:
    FileInfo fileinfo;
    bool may_seed = false;
};

#endif