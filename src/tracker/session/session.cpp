#include "session.h"

bool Session::add_table(const std::string& hash, IPTable& peertable) {
    return peertables.insert({hash, peertable}).second;
}

bool Session::add_peer(const std::string& hash, const Address& peer) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 
    return it->second.add_ip(peer);
}


bool Session::get_table(const std::string& hash, IPTable& peertable) const {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 

    peertable = it->second;
    return true;
}


bool Session::remove_peer(const std::string& hash, const Address& peer) {
    return remove_peer(hash, peer.ip);
}


bool Session::remove_peer(const std::string& hash, const std::string& peer) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 

    it->second.remove_ip(peer);
    return true; 
}

bool Session::remove_table(const std::string& hash) {
    auto it = peertables.find(hash);
    if (it == peertables.end())
        return false; 
    peertables.erase(it);
    return true;
}

void Session::garbage_collect() {
    //TODO @Sebastiaan: IDEA: Add a Deletable template/interface to values of peertables map.
    // That has to keep a dirty bool. Must check/update dirty bool when fetching/storing data.
    // std::vector<IPTable> new_table(idmap.size());

    // unsigned i = 0; 
    // for (auto& item : idmap) {
    //     new_table[i] = peertables[item.second];
    //     item.second = i++;
    // }

    // peertables = new_table;
}