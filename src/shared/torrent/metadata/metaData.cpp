#include <string>

#include "metaData.h"

void TorrentMetadata::read_stream(std::istream& stream) {
    // Read name
    unsigned name_length;
    stream.read((char*)(&name_length), sizeof(name_length));
    name.resize(name_length);
    stream.read((char*)name.data(), name_length);

    // Read content_hash
    unsigned hash_length; 
    stream.read((char*)(&hash_length), sizeof(hash_length));
    content_hash.resize(hash_length);
    stream.read((char*)content_hash.data(), hash_length);

    // Read size
    stream.read((char*)(&size), sizeof(size));

    // Read fragment_size
    stream.read((char*)(&fragment_size), sizeof(fragment_size));
}

void TorrentMetadata::write_stream(std::ostream& stream) const {
    // Write name
    unsigned name_length = name.size();
    stream.write((char*)(&name_length), sizeof(name_length));
    stream.write((char*)name.data(), name_length);

    // Write content_hash
    unsigned hash_length = content_hash.size();
    stream.write((char*)&hash_length, sizeof(hash_length));
    stream.write((char*)content_hash.data(), hash_length);

    // Write size
    stream.write((char*)(&size), sizeof(size));

    // Write fragment_size
    stream.write((char*)(&fragment_size), sizeof(fragment_size));
}