#include "shared/torrent/ipTable/address.h"

#include "registry.h"

void torrent::peer::Registry::add(const Address& address, size_t num_fragments) {
    peers.insert({address.ip, torrent::peer::Registry::Element(address, num_fragments)});
}