#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "peer/connections/tracker/tracker.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"
#include "torrent.h"

bool torrent::run(const std::string& torrentfile) {   
    // 1. Load trackerlist from tf
    TorrentFile tf = TorrentFile::from(torrentfile);
    const IPTable& table = tf.get_trackertable();

    std::vector<int> trackers_placeholder = {1, 2, 3, 4};
    // 2. Connect to trackers in list
    for (auto t : trackers_placeholder) {
        auto address = "placeholder";
        uint16_t  port = 2323; //placeholder
        auto tracker_conn = TCPClientConnection::Factory::from(NetType::IPv4).withAddress(address).withPort(port).create();
        if (tracker_conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
        if (!tracker_conn->doConnect()) {
            std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }

        auto hash = "@Mariska, how do we generate the torrent hash for the TorrentFile tf here?";

        IPTable peertable;
        // 3. Request trackers to provide peertables
        // 4. Receive peertables
        if (!connections::tracker::receive(tracker_conn, hash, peertable)) {
            std::cerr<<"Could not send RECEIVE request to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
        // 5. Unify peertables
        //TODO @Sebastiaan vectorize this for loop, make parallel processing tasks.
        // Pro: We can use a mergesort tree structure to merge peertables, which is big fast!
        // Con: This is nontrivial to implement, needs multiprocessing preferably. 
        // That would mean: forking or popen syscalls... Or std::async!
    }
    
    // 6. Pick a bunch of peers to request filedata from

    return true;
}

bool torrent::make(std::string in, std::string out, std::vector<std::string>& trackers) {
    try {
        IPTable table = IPTable::from(trackers);
        TorrentFile::make_for(table, in).save(out);  
    } catch (const std::exception& e) {
        std::cerr << print::RED << "[ERROR] " << e.what() << std::endl;
        return false;
    }
    return true;
}