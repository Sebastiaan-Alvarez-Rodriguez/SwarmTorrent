#include <fstream>

#include "shared/util/fs/fs.h"
#include "shared/util/hash/hasher.h"

#include "shared/torrent/file/defaults.h"
#include "hashTable.h"

HashTable HashTable::make_for(const std::string& path) {
    uint64_t fragment_size = torrent::file::defaults::fragment_size;
    if (!fs::is_file(path))
        throw std::runtime_error("Can only open files for now!");

    uint64_t f_size = fs::file_size(path);
    uint64_t nr_fragments = ((f_size -1) / fragment_size) + 1;
    HashTable hashTable;

    hashTable.hashes.reserve(nr_fragments);
    std::ifstream f(path, std::ios::in | std::ios::binary);
    for (uint64_t i = 0; i < nr_fragments; ++i) {
        uint64_t size = (i != nr_fragments-1) ? fragment_size : (f_size % fragment_size);
        if (f_size % fragment_size == 0)
            size = fragment_size;
        const uint8_t* data = new uint8_t[size];
        f.read((char*) data, size);
        std::string hash;
        hash::sha256(hash, data, size);
        delete[] data;
        if (!hashTable.add_hash(hash)) {
            f.close();
            throw std::runtime_error("Creating HashTable failed");
        }
    }
    f.close();
    return hashTable;
}

bool HashTable::add_hash(const std::string& hash) {
    if (hash.length() != hash_type)
        return false;
    hashes.push_back(hash);
    return true;
}

void HashTable::write_stream(std::ostream& os) const {
    os.write((char*) &hash_type, sizeof(hash_type));
    size_t amount = hashes.size();
    os.write((char*) &amount, sizeof(amount));
    const auto hash_size = hash::size_for(hash_type);
    for (auto hash : hashes)
        os.write((char*) hash.data(), hash_size);
}

void HashTable::read_stream(std::istream& is) {
    is.read((char*) &hash_type, sizeof(hash_type));
    size_t amount;
    is.read((char*) &amount, sizeof(amount)); 
    hashes.resize(amount);

    const auto hash_size = hash::size_for(hash_type);
    for (unsigned i = 0; i < amount; ++i) {
        hashes[i].resize(hash_size);
        is.read((char*) hashes[i].data(), hash_size);
    }
}

bool HashTable::check_hash(unsigned index, const std::string& hash) const {
    return hash == hashes.at(index);
}