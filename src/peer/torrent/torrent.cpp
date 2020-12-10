#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "peer/connection/peer/connections.h"
#include "peer/connection/message/peer/message.h"
#include "peer/connection/tracker/connections.h"
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

// Send join requests to a number of peers. 
// We stop once there are no more peers left in `options`, or if we reach `needed_peers` peers.
// Returns true if we reached `needed_peers`, false otherwise
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
            std::cerr << print::YELLOW << "[WARN] Only implemented support for TCP connections, skipping: " << type << ": " << ip << ':' << port << '\n';
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
        uint16_t sourcePort = session.get_conn()->getSourcePort();
        if (!connections::peer::join(conn, sourcePort, torrent_hash)) {
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

bool torrent::run(const std::string& torrentfile, const std::string& workpath, uint16_t sourcePort) {
    // 0. Prepare and check output location
    if (!fs::mkdir(workpath)) {
        std::cerr << print::RED << "[ERROR] Could not construct path '" << workpath << "'" << print::CLEAR << std::endl;
        return false;
    }
    // 1. Load trackerlist from tf
    TorrentFile tf = TorrentFile::from(torrentfile);
    const IPTable& tracker_table = tf.getTrackerTable();

    //TODO: must get hash. Hash in TorrentFile?
    // std::string hash = generate_hash(torrentfile);
    std::string hash = "test";
    IPTable table = compose_peertable(hash, tracker_table, sourcePort);
    if (table.size() == 0) // We have a dead torrent
        return false;

    // 6. Construct torrent session (to maintain received fragments)
    auto session = torrent::Session(tf, TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(sourcePort).withBlocking(false).create());
    
    // 7. Initialize peer network: send requests to a number of peers to join.
    // TODO: Need reasonable cap on connected peers. 
    // In reality, cap depends on network bandwidth: Keep accepting, until bandwidth is filled up.
    const unsigned needed_peers = 1;
    join_peers(session, table, needed_peers);


    auto metadata = tf.getMetadata();
    const std::string fileloc = workpath + metadata.name;
    auto fragmentHandler = FragmentHandler(metadata, fileloc);

    bool stop = false;


    // Continually send and recv data.
    // Best approach might be to use 2 threads (1 for send, 1 for recv). For now, sequential is good enough.
    // We should use the same connection (and thus port) for requesting and receiving data, because otherwise, we would have to disconnect and reconnect a lot.
    // TODO: If we have a dead torrent, return false? Or periodically request trackers for new peertable

    const uint32_t max_outstanding_fragment_requests = 10;
    while (!stop) {

        // if (torrent is dead) {
        //     Request for new peertable? Keep handling requests below though!
        // }

        // 9. Send requests to get data
        //TODO: 2 options for sending
        // 1. Send using a timeout, 1 by 1. Pro is that we can use 1 port. Con is that 1-by-1 sending is slow.
        // 2. Same as 1, but using multiple threads. Pro is big performance, con is that we use multiple ports.
        // For now we make 1. Adaption to 2 is simple enough to not be a waste of time.

        //TODO: 2 options for handling receiving fragments
        // 1. Use connection used by the request to send back the data. Pro: easy to make. Con: Makes sending and receiving non-separable
        // 2. Send received fragments to a specific port. Pro: Separates sending and receiving. Con: We have to somehow remember which requests we sent out.
        //    In order to maintain which requests we sent, need a registry mapping fragment to Address.
        //    Because we request fragments only once, we should have unique keys.
        //    The registry needs a notion of timing: "When was request sent?"
        //    This is the only way to check for dead requests, and clean them up.
        //    Efficiently finding dead requests is pretty much impossible, unless we find some very smart data structure.

        // 10. Handle incoming connections
        const auto req_conn = session.get_conn();
        auto connection = req_conn->acceptConnection();
        if (connection == nullptr) {
            if (req_conn->get_state() == Connection::ERROR) {
                std::cerr << "Experienced error when checking for inbound communication: ";req_conn->print(std::cerr); std::cerr << '\n';
            } else {
                // Connection is not used at this time
                // TODO: Sleep for a little bit here
            }
            continue;
        } else { // We are dealing with an actual connection
            //TODO: Check what kind of connection we have!
            // Should be one of:
            // 1. join request
            // 2. leave message
            // 3. request for a fragment
            // 4. delivery of a fragment
            message::standard::Header standard;
            if (!message::standard::recv(connection, standard)) {
                std::cout << "Unable to peek. System hangup?" << std::endl;
                continue;
            }
            if (standard.formatType != message::peer::id) {
                std::cerr << "Received invalid message! Not a Peer-message. Skipping..." << std::endl;
                continue;
            }
            uint8_t* const data = (uint8_t*) malloc(standard.size);
            connection->recvmsg(data, standard.size);
            message::peer::Header* header = (message::peer::Header*) data;
            switch (header->tag) {
                case message::peer::EXCHANGE_REQ: break;
                case message::peer::EXCHANGE_CLOSE: break;
                case message::peer::DATA_REQ: break;
                case message::peer::DATA_REPLY: 
                // TODO: Write data using fragmentHandler, after checking of course
                // std::string hash;
                // hash::sha256(hash, data, data_size);
                // if (!torrentfile.check_hash(index, hash))
                //     return false;
                    break;
                default: // We get here when testing
                    break;
            }
            free(data);
        }
    }
    return true;
}