#include "hashTable.h"

bool HashTable::add_hash(std::string hash) {
    if (hash.length() != hash_type)
        return false;
    hashes.push_back(hash);
    return true;
}

void HashTable::write_swarm(std::ostream& os) const {
    os.write((char*)(&hash_type), sizeof(hash_type));
    unsigned size = hashes.size();
    os.write((char*)(&size), sizeof(size));
    for (auto hash : hashes) 
        os.write((char*)hash.data(), hash_type);
}

void HashTable::read_swarm(std::istream& is) {
    is.read((char*)hash_type, sizeof(hash_type));
    unsigned size;
    is.read((char*)(&size), sizeof(size)); 
    hashes.resize(size);
    for (unsigned i = 0; i < size; ++i)
        is.read((char*)hashes[i].data(), hash_type);
}

bool HashTable::check_hash(unsigned index, std::string hash) const {
    return hash == hashes.at(index);
}