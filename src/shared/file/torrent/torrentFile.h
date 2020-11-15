#ifndef TORRENTFILE_H
#define TORRENTFILE_H

#include <string>

#include "trackerTable/trackerTable.h"
#include "hashTable/hashTable.h"

// Object responsible for reading and writing (.st) torrent files
class TorrentFile {
public:
    //Constructs torrentfile by providing a path. Path should point to a .st file 
    TorrentFile(std::string path);
    //Constructs torrentfile by providing base information 
    TorrentFile(std::string name, unsigned length, unsigned fragment_length, unsigned seed_threshold) : name(name), length(length), fragment_length(fragment_length), seed_threshold(seed_threshold) {};
    // Constructs torrentfile by providing full information 
    TorrentFile(TrackerTable& trackertable, std::string name, unsigned length, unsigned fragment_length, unsigned seed_threshold, HashTable hashtable) : trackertable(trackertable), name(name), length(length), fragment_length(fragment_length), seed_threshold(seed_threshold), hashtable(hashtable) {};

    //Write the contents of the TorrentFile 
    void write(std::string path);

    //Returns a constant reference to the trackertable
    const TrackerTable& get_trackertable() const { return trackertable; };
    //Returns the suggested name to save the file
    std::string get_advise_name() const { return name; };
    //Returns the size of the file in bytes
    unsigned get_file_size() const { return length; }; 
    //Returns the number of fragments the file has
    unsigned get_nr_fragments() const { return length / fragment_length; };
    //Returns the size of the fragments in bytes
    unsigned get_fragment_size() const { return fragment_length; };
    //Returns if the peer has enough fragments to start seeding
    bool can_seed(unsigned nr_fragments) const { return nr_fragments >= seed_threshold; };
    //Check if the hash is equal to the hash at the given index of the hashtable
    bool check_hash(unsigned index, std::string hash) const { return hashtable.check_hash(index, hash);};
protected:
    //SwarmTorrent file currently associated with object
    std::string path;

private:
    //Contains information required to setup a socket with the trackers
    TrackerTable trackertable;
    //Suggested name to save the file (advisory)
    std::string name;
    //Total size of the file in bytes
    unsigned length;
    //Size in bytes for one fragment
    unsigned fragment_length;
    //Number of fragments required to start seeding
    unsigned seed_threshold;
    //SHA2's for every fragment
    HashTable hashtable;
};
#endif