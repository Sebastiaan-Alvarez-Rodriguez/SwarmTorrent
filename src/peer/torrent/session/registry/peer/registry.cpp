#include "shared/torrent/ipTable/address.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "peer/torrent/defaults.h"
#include "peer/connection/protocol/peer/connections.h"


#include "registry.h"

void torrent::peer::Registry::add(const Address& address, const std::vector<bool> fragments_completed) {
    peers.insert({address.ip, torrent::peer::Registry::Element(address, fragments_completed)});
}


void torrent::peer::Registry::mark(const std::string& ip) {
    auto it = peers.find(ip);
    if (it != peers.end())
        it->second.inactiveCounter = 0;
}

void torrent::peer::Registry::report(const std::string& ip) {
    auto it = peers.find(ip);
    if (it != peers.end())
        ++(it->second.inactiveCounter);
}

std::vector<Address> torrent::peer::Registry::get_peers_for(size_t fragment_nr) const {
    std::vector<Address> v;
    for (auto it = peers.cbegin(); it != peers.cend(); ++it)
        if (it->second.data_owned[fragment_nr])
            v.push_back(it->second.address);
    return v;
}

void torrent::peer::Registry::gc() {
    std::vector<std::string> to_remove; 
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        if (it->second.inactiveCounter < ::peer::defaults::torrent::inactive_threshold)
            continue;
        auto peer = (*it).second.address;
        auto peer_conn = TCPClientConnection::Factory::from(peer.type).withAddress(peer.ip).withDestinationPort(peer.port).create();
        if (!connections::peer::send::inquire(peer_conn))
            to_remove.push_back(peer.ip);
        else 
            it->second.inactiveCounter = 0;
    }

    for (auto peer : to_remove)
        peers.erase(peer);
}