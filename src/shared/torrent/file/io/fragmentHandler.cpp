#include <fstream>

#include "shared/util/hash/hasher.h"
#include "fragmentHandler.h"

FragmentHandler::~FragmentHandler() { 
    if (write_head.is_open())
        write_head.close();
    if (read_head.is_open()) 
        read_head.close(); 
}

//TODO: check if write failed?
bool FragmentHandler::write(unsigned index, const uint8_t* data, unsigned data_size) {
    if (!write_head.is_open())
        return false;

    //displacement with respect to previous write
    const unsigned displ = (index*fragment_size) - prev_write_index;
    prev_write_index = index*fragment_size;

    write_head.seekp(displ, std::ios_base::cur);
    write_head.write((char*)data, data_size);

    return true;
} 

//TODO: check if read failed?
bool FragmentHandler::read(unsigned index, uint8_t* data, unsigned& data_size) {
    if (!read_head.is_open())
        return false;

    //receive data_size and allocate required memory
    const unsigned frag_size = fragment_size;
    data_size = (index != num_fragments-1) ?  frag_size : file_size % frag_size;
    data = new uint8_t[data_size];

    //displacement with respect to previous read
    const unsigned displ = (index*frag_size) - prev_read_index; 
    prev_read_index = index*frag_size;

    read_head.seekg(displ, std::ios_base::cur);
    read_head.read((char*)data, data_size);

    return true;
}