#include "registry.h"

void tracker::torrent::Registry::create_table(const std::string& hash) {
    peertables.insert({hash, {IPTable()}});
}

bool tracker::torrent::Registry::add_table(const std::string& hash, IPTable&& peertable) {
    return peertables.insert({hash, {peertable}}).second;
}

bool tracker::torrent::Registry::add_peer(const std::string& hash, const Address& address, bool exist_ok) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 
    return it->second.table.add(address) || exist_ok;
}

bool tracker::torrent::Registry::get_table(const std::string& hash, IPTable& peertable) const {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 

    peertable = it->second.table;
    return true;
}

const auto tracker::torrent::Registry::last_checked(const std::string& hash) const {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        throw std::runtime_error("Could not get last_checked for non-existing table hash "+hash);
    return it->second.timestamp;
}


bool tracker::torrent::Registry::remove_peer(const std::string& hash, const Address& address) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 

    it->second.table.remove(address);
    return true; 
}

bool tracker::torrent::Registry::remove_table(const std::string& hash) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 
    peertables.erase(it);
    return true;
}
