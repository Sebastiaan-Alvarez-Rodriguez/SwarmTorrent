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
    const IPTable& getTrackerTable() const { return trackertable; };

    const HashTable& getHashTable() const { return hashtable; }

    const TorrentMetadata& getMetadata() const { return metadata; }
};

#endif