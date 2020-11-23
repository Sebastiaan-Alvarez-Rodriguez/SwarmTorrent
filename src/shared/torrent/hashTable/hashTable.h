#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string>
#include <vector>
#include <iostream>

#include "shared/torrent/file/streamable/streamable.h"
#include "shared/util/hash/hasher.h"

class HashTable : public Streamable {
public:
    // Constructs a HashTable with given hash type
    HashTable(hash::Hash hash_type) : hash_type(hash_type) {};

    // Constructs a HashTable with default hash type
    HashTable() : hash_type(hash::SHA256_H) {};

    // Constructs a hashtable for given file
    static HashTable make_for(std::string path);

    // Adds a hash to the table, if the size conforms to the hash type
    // Returns if success
    bool add_hash(std::string hash);
    // Check if the hash is equal to the hash at the given index of the hashtable
    bool check_hash(unsigned index, std::string hash) const; 

    // Reads and writes the hashtable to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    void read_stream(std::istream& is) override;

private: 
    const hash::Hash hash_type;
    std::vector<std::string> hashes;    
};

#endif