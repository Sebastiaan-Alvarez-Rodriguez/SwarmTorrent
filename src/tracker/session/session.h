#ifndef TRACKER_SESSION_H
#define TRACKER_SESSION_H

#include <vector>
#include <unordered_map>

#include "shared/torrent/ipTable/ipTable.h"

class Session {
public:
    Session() = default;

    // Add a table with hash as identifier. Returns true on insertion, false when it already exists.
    bool add_table(const std::string& hash, IPTable& peertable);

    // Get the table with hash as identifier
    bool get_table(const std::string& hash, IPTable& peertable) const;

    // Removes table with hash as identifier
    // Note: to keep idmap valid, this does not remove the
    // table from the peertables vector
    // perform garbage collect once in a while to clean 
    // up.
    bool remove_table(const std::string& hash);

    // Add a peer to the table with hash as identifier. 
    // Returns true on insertion, false when IPTable not found or peer already added.
    bool add_peer(const std::string& hash, const Address& peer);

    
    // Update the table with hash as identifier
    // TODO: diffTable?
    //bool update_table(std::string hash, );
    // Remove peer from table with hash as identifier
    bool remove_peer(const std::string& hash, const Address& peer);

    // Remove peer from table with hash as identifier
    bool remove_peer(const std::string& hash, const std::string& peer);


    // Garbage collect the peertables vector
    void garbage_collect();

protected:
    // Maps the hashes of the torrentfile to its peertable registry
    std::unordered_map<std::string, IPTable> peertables;
};

#endif