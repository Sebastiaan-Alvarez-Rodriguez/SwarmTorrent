#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <fstream>

#include "peer/connections/peer/connections.h"
#include "peer/connections/tracker/connections.h"
#include "peer/torrent/session/session.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/message.h"
#include "shared/connection/meta/type.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"
#include "shared/util/fs/fs.h"
#include "shared/util/hash/hasher.h"
#include "torrent.h"



static IPTable compose_peertable(const std::string& hash, const IPTable& trackers, uint16_t sourcePort) {
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
        auto tracker_conn = TCPClientConnection::Factory::from(addr_info.type.n_type).withAddress(address).withSourcePort(sourcePort).withDestinationPort(port).create();
        if (tracker_conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!tracker_conn->doConnect()) {
            std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }

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

static bool join_peers(torrent::Session& session, const IPTable& options, unsigned needed_peers) {
    auto torrent_hash = "some placeholder hash"; //TODO: fetch hash from tf?
    // unsigned random_start = 0; 
    //TODO @Mariska: 
    // Need something to balance load between peers. Now all peers will probably connect to same others first.
    // Maybe make a random generator in util to generate some random number in a given range?
        // Initialize the random generator with something that is different for every peer... Hostname string? Hostname+ip?
        // Note that initializing with time is maybe/probably not good enough, so only do that if the above option won't work
    // Using iterator to pick a random starting point is inefficient-ish, but maybe the only choice:
        // https://stackoverflow.com/questions/15425442/

    IPTable accepted;
    for (auto it = options.iterator_begin(); it != options.iterator_end() && accepted.size() < needed_peers; ++it) {
        const auto& ip = it->first;
        uint16_t port = it->second.port;
        const auto& type = it->second.type;

        if (type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Only implemented support for TCP connections, skipping : " << type << ": " << ip << ':' << port << '\n';
            continue;
        }

        auto conn = TCPClientConnection::Factory::from(type.n_type).withAddress(ip).withDestinationPort(port).create();
        if (conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!conn->doConnect()) {
            std::cerr << "Could not connect to peer ";conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!connections::peer::join(conn, torrent_hash)) {
            std::cerr << print::YELLOW << "[WARN] Could not send join request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        message::standard::Header header;
        if (!message::standard::recv(conn, header)) {
            std::cerr <<print::YELLOW << "[WARN] Could not receive join request response from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (header.formatType == message::standard::OK) {
            accepted.add_ip({TransportType::TCP, NetType::IPv4}, ip, port);
        } else if (header.formatType == message::standard::REJECT) {
            std::cerr << print::CYAN << "[TEST] We got a REJECT for our join request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        } else {
            std::cerr <<print::YELLOW << "[WARN] Received non-standard-conforming response ("<<header.formatType<<") from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }
    return accepted.size() >= needed_peers;
}

bool torrent::make(const std::string& in, const std::string& out, std::vector<std::string>& trackers) {
    auto t_size = trackers.size(); 
    if (t_size == 0) {
        std::cerr << print::RED << "[ERROR] Cannot make torrentfile without any trackers given" << print::CLEAR << '\n';
        return false;
    }
    // try {
        IPTable table = IPTable::from(trackers);
        TorrentFile tf = TorrentFile::make_for(table, in);
        tf.save(out);
        std::string torrent_hash = tf.getMetadata().content_hash;
        if (torrent_hash == "") {
            std::cerr << print::RED << "[ERROR] TorrentFile could not be hashed properly" << print::CLEAR << '\n';
            return false;
        }


        IPTable connected;
        for (auto it = table.iterator_begin(); it != table.iterator_end(); ++it) {
            const std::string& ip = it->first;
            const auto addr = it->second;

            auto conn = TCPClientConnection::Factory::from(NetType::IPv4).withAddress(ip).withDestinationPort(addr.port).create();
            if (conn->get_state() != ClientConnection::READY) {
                std::cerr << print::YELLOW << "[WARN] Could not initialize connection to tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
                continue;
            }
            if (!conn->doConnect()) { // TODO: Need timeout here probably
                std::cerr << "Could not connect to tracker ";conn->print(std::cerr);std::cerr << '\n';
                continue;
            }
            if (!connections::tracker::make_torrent(conn, torrent_hash)) { // TODO: Need timeout here probably
                std::cerr << print::YELLOW << "[WARN] Could not send torrent request for tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
                continue;
            }

            message::standard::Header h;
            if (message::standard::recv(conn, h) && h.formatType == message::standard::OK) {
                connected.add_ip(addr);
            } else {
                std::cerr << print::YELLOW << "[WARN] No confirming message received from tracker: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            }
        }
        if (connected.size() == 0) {
            std::cerr << print::RED << "[ERROR] No successful connections to any tracker" << print::CLEAR << '\n';
            return false;
        } else if (connected.size() < t_size) {
            std::cerr << print::YELLOW << "[WARN] Could only register at "<< connected.size() << '/' << t_size << " trackers" << print::CLEAR << '\n';
            //TODO: Maybe should mention which trackers were the failed ones
        } else {
            std::cerr << print::GREEN << "[SUCCESS] Registered torrentfile-in-progress at " << connected.size() << '/' << t_size << " trackers" << print::CLEAR << '\n';
        }

         
    // } catch (const std::exception& e) {
    //     std::cerr << print::RED << "[ERROR] " << e.what() << std::endl;
    //     return false;
    // }
    return true;
}

bool torrent::run(const std::string& torrentfile, const std::string& workpath, uint16_t sourcePort) {
    // 0. Prepare and check output location
    if (!fs::mkdir(workpath)) {
        std::cerr << print::RED << "[ERROR] Could not construct path '" << workpath << "'" << print::CLEAR << std::endl;
        return false;
    }
    // 1. Load trackerlist from tf
    TorrentFile tf = TorrentFile::from(torrentfile);
    const IPTable& tracker_table = tf.getTrackerTable();

    //TODO: hash in TorrentFile?
    // std::string hash = generate_hash(torrentfile);
    std::string hash = "test";
    IPTable table = compose_peertable(hash, tracker_table, sourcePort);
    if (table.size() == 0) // We have a dead torrent
        return false;

    // 6. Construct torrent session (to maintain received fragments)
    auto session = torrent::Session(tf);

    // 7. Initialize peer network: send requests to a number of peers to exchange data
    // TODO: Need reasonable cap on connected peers. 
    // In reality, cap depends on network bandwidth: Keep accepting, until bandwidth is filled up.
    const unsigned needed_peers = 1;
    join_peers(session, table, needed_peers);


    auto metadata = tf.getMetadata();
    const std::string fileloc = workpath + metadata.name;
    auto fragmentHandler = FragmentHandler(tf.getMetadata(), fileloc);

    bool stop = false;


    while (!stop) {

        // 8. After building a large enough network, we must continually send and recv data.
        //    Best approach is to use 2 threads (1 for send, 1 for recv). For now, sequential is good enough.
        //    We should use different ports for requesting and receiving data, because these things do not wait nicely on each other.

        // 8. Request filedata, receive fragments
        // 8b. If we have a dead torrent, return false? Or periodically request trackers for new peertable
        // 9. Write data using fragmentHandler, after checking of course
        
        // std::string hash;
        // hash::sha256(hash, data, data_size);
        // if (!torrentfile.check_hash(index, hash))
        //     return false;
        // 10. Send filedata to others, if we own the required data
        stop = true;
    }
    return true;
}