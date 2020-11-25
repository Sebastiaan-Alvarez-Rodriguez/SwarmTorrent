#ifndef TRACKER_H
#define TRACKER_H 

#include <vector>
#include <unordered_map>

#include "shared/torrent/ipTable/ipTable.h"

class Tracker {
private:
    std::vector<IPTable> peertables;
    // Maps the hashes of the torrentfile to the index in the peertables vector
    std::unordered_map<std::string, unsigned> idmap;


public:
    Tracker() {};

    bool add_table(std::string hash, IPTable& peertable);
    bool add_peer(std::string hash, Addr peer);
    bool get_table(std::string hash, IPTable& peertable);
    //TODO: difTable?
    //bool update_table(std::string hash, );
    bool remove_peer(std::string hash, Addr peer);
    bool remove_peer(std::string hash, std::string peer);
    // Note: to keep idmap valid, this does not remove the
    // table from the peertables vector
    // perform garbage collect once in a while to clean 
    // up.
    bool remove_table(std::string hash);

    void garbage_collect();
};

#endif