#include "peerTable.h"

IPTable peertable::init(std::string initial_seeder) {
    std::vector<std::string> peers = {initial_seeder};
    return IPTable::from(peers);
}