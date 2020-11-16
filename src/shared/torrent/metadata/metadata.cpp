#include <iostream>
#include <string>

#include "metadata.h"

void TorrentMetadata::read_stream(std::istream& stream) {
    //TODO: Stream.read() can probably be replaced with <</>>
    // for constant-size types at least
    unsigned name_length;
    stream.read((char*)(&name_length), sizeof(name_length));
    stream.read((char*)name.data(), name_length);
    stream.read((char*)(&size), sizeof(size));
    stream.read((char*)(&fragment_size), sizeof(fragment_size));
}

void TorrentMetadata::write_stream(std::ostream& stream) const {
    //TODO: Stream.write() can probably be replaced with <</>>
    // for constant-size types at least 
    unsigned name_length = name.length();
    stream.write((char*)(&name_length), sizeof(name_length));
    stream.write((char*)name.data(), name_length);
    stream.write((char*)(&size), sizeof(size));
    stream.write((char*)(&fragment_size), sizeof(fragment_size));
}