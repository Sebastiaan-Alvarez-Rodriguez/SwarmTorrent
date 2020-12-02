#include "tracker.h"

bool Tracker::add_table(const std::string& hash, IPTable& peertable) {
    if (idmap.find(hash) != idmap.end())
        return false;

    unsigned index = peertables.size();
    peertables.push_back(peertable);
    idmap.insert({hash, index});
    return true;
}

bool Tracker::add_peer(const std::string& hash, Addr peer) {
    auto it = idmap.find(hash);
    if (it == idmap.end())
        return false; 

    IPTable peertable = peertables[(*it).second];
    return peertable.add_ip(peer);
}


bool Tracker::get_table(const std::string& hash, IPTable& peertable) {
    auto it = idmap.find(hash);
    if (it == idmap.end())
        return false; 

    peertable = peertables[(*it).second];
    return true;
}


bool Tracker::remove_peer(const std::string& hash, Addr peer) {
    auto it = idmap.find(hash);
    if (it == idmap.end())
        return false; 

    IPTable peertable = peertables[(*it).second];
    peertable.remove_ip(peer);
    return true;
}


bool Tracker::remove_peer(const std::string& hash, std::string peer) {
    auto it = idmap.find(hash);
    if (it == idmap.end())
        return false; 

    IPTable peertable = peertables[(*it).second];
    peertable.remove_ip(std::move(peer));
    return true; 
}

bool Tracker::remove_table(const std::string& hash) {
    auto it = idmap.find(hash);
    if (it == idmap.end())
        return false; 
    idmap.erase(it);
    return true;
}

void Tracker::garbage_collect() {
    std::vector<IPTable> new_table(idmap.size());

    unsigned i = 0; 
    for (auto& item : idmap) {
        new_table[i] = peertables[item.second];
        item.second = i++;
    }

    peertables = new_table;
}