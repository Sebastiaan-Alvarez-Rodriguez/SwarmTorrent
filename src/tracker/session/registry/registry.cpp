#include "registry.h"

void torrent::tracker::Registry::create_table(const std::string& hash) {
    peertables.insert({hash, {IPTable()}});
}

bool torrent::tracker::Registry::add_table(const std::string& hash, IPTable&& peertable) {
    return peertables.insert({hash, {peertable}}).second;
}

bool torrent::tracker::Registry::add_peer(const std::string& hash, const Address& peer, bool exist_ok) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 
    return it->second.table.add_ip(peer) || exist_ok;
}

bool torrent::tracker::Registry::get_table(const std::string& hash, IPTable& peertable) const {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 

    peertable = it->second.table;
    return true;
}

const auto torrent::tracker::Registry::last_checked(const std::string& hash) const {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        throw std::runtime_error("Could not get last_checked for non-existing table hash "+hash);
    return it->second.timestamp;
}


bool torrent::tracker::Registry::remove_peer(const std::string& hash, const Address& peer) {
    return remove_peer(hash, peer.ip);
}


bool torrent::tracker::Registry::remove_peer(const std::string& hash, const std::string& peer) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 

    it->second.table.remove_ip(peer);
    return true; 
}

bool torrent::tracker::Registry::remove_table(const std::string& hash) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 
    peertables.erase(it);
    return true;
}
