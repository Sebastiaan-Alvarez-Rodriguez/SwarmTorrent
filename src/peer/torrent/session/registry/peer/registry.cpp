#include "peer/connection/protocol/peer/connections.h"
#include "peer/torrent/defaults.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/torrent/ipTable/address.h"


#include "registry.h"


void peer::torrent::PeerRegistry::mark(const Address& address) {
    auto it = peers.find(address);
    if (it != peers.end())
        it->second.inactiveCounter = 0;
}

void peer::torrent::PeerRegistry::report(const Address& address) {
    auto it = peers.find(address);
    if (it != peers.end())
        ++(it->second.inactiveCounter);
}

std::vector<Address> peer::torrent::PeerRegistry::get_peers_for(size_t fragment_nr) const {
    std::vector<Address> v;
    for (auto it = peers.cbegin(); it != peers.cend(); ++it)
        if (it->second.data_owned[fragment_nr])
            v.push_back(it->first);
    return v;
}

void peer::torrent::PeerRegistry::gc() {
    std::vector<Address> to_remove; 
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        if (it->second.inactiveCounter < ::peer::torrent::defaults::inactive_threshold)
            continue;
        const auto& address = it->first;
        auto peer_conn = TCPClientConnection::Factory::from(address.type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (!connections::peer::send::inquire(std::move(peer_conn)))
            to_remove.push_back(address);
        else 
            it->second.inactiveCounter = 0;
    }

    for (auto addr : to_remove)
        peers.erase(addr);
}