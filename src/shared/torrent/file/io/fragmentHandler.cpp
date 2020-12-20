#include <cstring>
#include <fstream>

#include "fragmentHandler.h"

FragmentHandler::~FragmentHandler() { 
    if (write_head.is_open())
        write_head.close();
    if (read_head.is_open()) 
        read_head.close(); 
}

uint8_t* FragmentHandler::read_with_leading(unsigned index, unsigned& data_size, size_t leading_size) {
    if (!read_head.is_open())
        return nullptr;

    //receive data_size and allocate required memory
    const unsigned frag_size = fragment_size;
    data_size = (index != num_fragments-1) ? frag_size : file_size % (frag_size+1);
    uint8_t* const data = (uint8_t*) malloc(data_size+leading_size);

    //move until after the packet header to allocate
    uint8_t* const ptr = data + leading_size;

    //displacement with respect to previous read
    const int64_t displ = (int64_t) (index*frag_size) - (int64_t) prev_read_index; 
    prev_read_index = index*frag_size + data_size; // After reading, readhead is at end of fragment we just read
    read_head.seekg(displ, std::ios_base::cur);
    read_head.read((char*) ptr, data_size);

    return data;
}


bool FragmentHandler::write(unsigned index, const uint8_t* data, unsigned data_size) {
    if (!write_head.is_open())
        return false;
    if (index > num_fragments) {
        std::cerr << "Error: cannot write fragment " << index << ", only " << num_fragments << " exist\n";
        exit(1);
    }
    write_head.seekp(index*fragment_size, std::ios::beg);
    write_head.write((char*)data, data_size);

    return true;
}