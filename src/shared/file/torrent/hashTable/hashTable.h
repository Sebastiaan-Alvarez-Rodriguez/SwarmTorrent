#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string>
#include <vector>
#include <iostream>

#include "../../hash/hasher.h"
#include "../swarmTorrentWriter.h"

class HashTable : public SwarmTorrentWriter {
public:
    //Constructs a HashTable with given hash type
    HashTable(Hash hash_type) : hash_type(hash_type) {};
    //Constructs a HashTable with default hash type
    HashTable() : hash_type(SHA256_H) {};

    //Adds a hash to the table, if the size conforms to the hash type
    //Returns if success
    bool add_hash(std::string hash);
    //Check if the hash is equal to the hash at the given index of the hashtable
    bool check_hash(unsigned index, std::string hash) const; 

    //Reads and writes the hashtable to a SwarmTorrent file
    void write_swarm(std::ostream& os) const override;
    void read_swarm(std::istream& is) override;

private: 
    const Hash hash_type;
    std::vector<std::string> hashes;    
};

#endif