#ifndef TORRENTFILE_H
#define TORRENTFILE_H

#include <cstdint>
#include <string>

#include "shared/torrent/hashTable/hashTable.h"
#include "shared/torrent/metadata/metaData.h"
#include "shared/torrent/ipTable/ipTable.h"

// Object responsible for reading and writing (.st) torrent files
class TorrentFile {
protected:
    IPTable trackertable; // Information to connect with trackers
    TorrentMetadata metadata; // Information about the streamed file
    HashTable hashtable; // SHA256's for every fragment
public:
    TorrentFile(IPTable& tt, TorrentMetadata& tm, HashTable& ht) : trackertable(tt), metadata(tm), hashtable(ht) {}
    
    // Constructs a torrentfile by reading file at given path
    static TorrentFile from(const std::string& path);

    // Constructs a torrentfile in-memory representation for given path
    // Note: Only files are currently supported
    static TorrentFile make_for(IPTable& tb, const std::string& path);


    // Write the contents of the TorrentFile 
    void save(const std::string& path) const;


    // Returns a constant reference to the trackertable
    const IPTable& get_trackertable() const { return trackertable; };
    
    // Returns the suggested name to save the file
    std::string get_advise_name() const { return metadata.name; };
    
    // Returns the size of the file we stream, in bytes
    uint64_t size() const { return metadata.size; }; 
    
    // Returns the number of fragments the file has
    uint64_t get_nr_fragments() const { return ((metadata.size-1) / metadata.fragment_size) + 1; };
    
    // Returns the size of the fragments in bytes
    uint64_t get_fragment_size() const { return metadata.fragment_size; };
  
    // Check if the hash is equal to the hash at the given index of the hashtable
    bool check_hash(unsigned index, std::string hash) const { return hashtable.check_hash(index, hash);};
};

#endif