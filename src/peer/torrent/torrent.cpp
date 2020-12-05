#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "peer/connections/tracker/connections.h"

#include "peer/torrent/session/session.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/message.h"
#include "shared/connection/meta/type.h"
#include "shared/torrent/file/io/fragmentHandler.h"
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
    const IPTable& tracker_table = tf.getTrackerTable();

    IPTable table = compose_peertable(tracker_table);
    if (table.size() == 0) // We have a dead torrent
        return false;

    // 6. Construct torrent session (to maintain received fragments)
    auto session = torrent::Session(tf);

    bool stop = false;
    while (!stop) {
        // 7. Pick a bunch of peers to request filedata from!

        // 8. Request filedata, receive fragments
        // 8b. If we have a dead torrent, return false? Or periodically request trackers for new peertable
        // 9. Write data using fragmentHandler, after checking of course

        std::string path_placeholder = "/tmp/oof.output"; //TODO: Intelligently construct full file paths here
        auto fragmentHandler = FragmentHandler(tf.getMetadata(), path_placeholder);
        // std::string hash;
        // hash::sha256(hash, data, data_size);
        // if (!torrentfile.check_hash(index, hash))
        //     return false;
        // 10. Send filedata to others, if we own the required data
        stop = true;
    }
    return true;
}

bool torrent::make(const std::string& in, const std::string& out, std::vector<std::string>& trackers) {
    auto t_size = trackers.size(); 
    if (t_size == 0) {
        std::cerr << print::RED << "[ERROR] Cannot make torrentfile without any trackers given" << print::CLEAR << '\n';
        return false;
    }

    auto torrent_hash = "some placeholder hash"; //TODO @Mariska: Implement hashing-one-liner
    // try {
        IPTable table = IPTable::from(trackers);
        size_t success = 0;
        for (auto it = table.iterator_begin(); it != table.iterator_end(); ++it) {
            const auto& ip = it->first;
            uint16_t port = it->second.port;

            auto conn = TCPClientConnection::Factory::from(NetType::IPv4).withAddress(ip).withPort(port).create();
            if (conn->get_state() != ClientConnection::READY) {
                std::cerr << print::YELLOW << "[WARN] Could not initialize connection to tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
                table.remove_ip(it->first);
                continue;
            }
            if (!conn->doConnect()) {
                std::cerr << "Could not connect to tracker ";conn->print(std::cerr);std::cerr << '\n';
                table.remove_ip(it->first);
                continue;
            }
            if (!connections::tracker::make_torrent(conn, torrent_hash)) {
                std::cerr << print::YELLOW << "[WARN] Could not send torrent request for tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
                table.remove_ip(it->first);
                continue;
            }

            message::standard::Header h;
            if (message::standard::from(conn, h) && h.formatType == message::standard::type::OK) {
                ++success;
            } else {
                std::cerr << print::YELLOW << "[WARN] No confirming message received from tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
                table.remove_ip(it->first);
            }
        }
        if (success == 0) {
            std::cerr << print::RED << "[ERROR] No successful connections to any tracker" << print::CLEAR << '\n';
            return false;
        } else if (success < t_size) {
            std::cerr << print::YELLOW << "[WARN] Could only register at "<< success << '/' << t_size << " trackers" << print::CLEAR << '\n';
            //TODO: Maybe should mention which trackers were the failed ones
        } else {
            std::cerr << print::GREEN << "[SUCCESS] Registered torrentfile-in-progress at " << success << '/' << t_size << " trackers" << print::CLEAR << '\n';
        }

        TorrentFile::make_for(table, in).save(out);  
    // } catch (const std::exception& e) {
    //     std::cerr << print::RED << "[ERROR] " << e.what() << std::endl;
    //     return false;
    // }
    return true;
}