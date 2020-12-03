#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "peer/connections/tracker/connections.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/meta/type.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"
#include "torrent.h"



static IPTable compose_peertable(const IPTable& trackers) {
    std::vector<IPTable> peertables(trackers.size());

    // 2. Connect to trackers in list
    size_t idx = 0;
    for (auto it = trackers.iterator_begin(); it != trackers.iterator_end(); ++it, ++idx) {
        auto address = it->first;
        auto addr_info = it->second;
        uint16_t port = addr_info.port;

        if (addr_info.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection for non-TCP transport type: '" << addr_info.type.t_type << '\'' << print::CLEAR << '\n';
            continue;
        }
        auto tracker_conn = TCPClientConnection::Factory::from(addr_info.type.n_type).withAddress(address).withPort(port).create();
        if (tracker_conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!tracker_conn->doConnect()) {
            std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }

        auto hash = "@Mariska, how do we generate the torrent hash for the TorrentFile tf here?";

        // 3. Request trackers to provide peertables
        // 4. Receive peertables
        if (!connections::tracker::receive(tracker_conn, hash, peertables[idx])) {
            std::cerr<<"Could not send RECEIVE request to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
    }
    // 5. Unify peertables
    auto maintable = peertables[0];
    for (idx = 1; idx < peertables.size(); ++idx)
        maintable.merge(peertables[idx]);

    //TODO @Sebastiaan: vectorize above for-loop, make parallel processing tasks?
    // Maybe even the for-loop above that, so each process fetches its own table as well, and we simultaneously ask tables.
    // Pro: We can use a mergesort tree structure to merge peertables, which is big fast!
    // Con: This is nontrivial to implement, needs multiprocessing preferably. 
    // That would mean: forking or popen syscalls... Or std::async!
    //  First check: Is it needed? Depends on amount of trackers, and the peertable sizes they return
    return maintable;
}

bool torrent::run(const std::string& torrentfile) {   
    // 1. Load trackerlist from tf
    TorrentFile tf = TorrentFile::from(torrentfile);
    const IPTable& tracker_table = tf.get_trackertable();

    IPTable table = compose_peertable(tracker_table);
    // 6. Construct torrent session (to maintain received fragments)
    // 7. Pick a bunch of peers to request filedata from!
    // 8. Request filedata, receive fragments, win!
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