#include "shared/torrent/ipTable/address.h"

#include "registry.h"

void torrent::peer::Registry::add(const Address& address, const std::vector<bool> fragments_completed) {
    peers.insert({address.ip, torrent::peer::Registry::Element(address, fragments_completed)});
}


void torrent::peer::Registry::mark(const std::string& ip) {
    auto it = peers.find(ip);
    if (it != peers.end())
        it->second.timestamp = std::chrono::steady_clock::now();
}