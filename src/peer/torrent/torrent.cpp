#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "shared/connection/impl/TCP/out/TCPOutConnection.h"
#include "shared/torrent/file/torrentFile.h"
#include "torrent.h"

bool torrent::run(uint16_t port) {
    std::string tracker_addr = "0.0.0.0"; //TODO get addr from trackerlist
    uint16_t tracker_port = 1042; //TODO: get port from trackerlist
   
    auto tracker_conn = TCPOutConnection::Factory::from(NetType::IPv4).withAddress(tracker_addr).withPort(tracker_port).create();
    if (tracker_conn->get_state() != Connection::READY) {
        std::cerr << print::RED << "[ERROR] Could not initialize connection" << std::endl;
        return false;
    }

    //TODO: 
    // 1. Load trackerlist from tf
    // 2. Connect to trackers in list
    // 3. Request trackers to provide peertables
    // 4. Receive peertables
    // 5. Unify peertables
    // 6. Pick a bunch of peers to request filedata from

    return true;
}

bool torrent::make(std::string in, std::string out, std::vector<std::string> trackers) {
    try {
        IPTable table = IPTable::from(trackers);
        TorrentFile::make_for(table, in).save(out);  
    } catch (std::exception e) {
        std::cerr << print::RED << "[ERROR]" << e.what() << std::endl;
        return false;
    }
    return true;
}