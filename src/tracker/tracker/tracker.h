#ifndef TRACKER_H
#define TRACKER_H 

#include <vector>
#include <unordered_map>

#include "shared/torrent/ipTable/ipTable.h"

class Tracker {
public:
    Tracker() {};

    // Add a table with hash as identifier
    bool add_table(const std::string& hash, IPTable& peertable);

    // Add a peer to the table with hash as identifier
    bool add_peer(const std::string& hash, Addr peer);

    // Get the table with hash as identifier
    bool get_table(const std::string& hash, IPTable& peertable);

    // Update the table with hash as identifier
    // TODO: difTable?
    //bool update_table(std::string hash, );
    // Remove peer from table with hash as identifier
    bool remove_peer(const std::string& hash, Addr peer);

    // Remove peer from table with hash as identifier
    bool remove_peer(const std::string& hash, std::string peer);

    // Removes table with hash as identifier
    // Note: to keep idmap valid, this does not remove the
    // table from the peertables vector
    // perform garbage collect once in a while to clean 
    // up.
    bool remove_table(const std::string& hash);

    // Garbage collect the peertables vector
    void garbage_collect();

protected:
    std::vector<IPTable> peertables;
    // Maps the hashes of the torrentfile to the index in the peertables vector
    std::unordered_map<std::string, unsigned> idmap;
};

#endif