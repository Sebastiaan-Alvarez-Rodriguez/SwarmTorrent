#ifndef PEER_TABLE_H
#define PEER_TABLE_H 

#include "shared/torrent/ipTable/ipTable.h"

namespace peertable {
    IPTable init(std::string initial_seeder);
}

#endif