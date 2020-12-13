#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "peer/connection/message/peer/message.h"
#include "peer/connection/protocol/peer/connections.h"
#include "peer/connection/protocol/tracker/connections.h"
#include "peer/torrent/pipeline/pipe_ops.h"
#include "peer/torrent/session/session.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/message.h"
#include "shared/connection/meta/type.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"
#include "shared/util/fs/fs.h"
#include "torrent.h"

//TODO: Temporary function to send register requests.
// Inefficient, because we have to open a separate connection for that
static bool tmp_send_register(ConnectionType type, std::string address, uint16_t port, std::string hash, uint16_t sourcePort) {
    auto tracker_conn = TCPClientConnection::Factory::from(type.n_type).withAddress(address).withDestinationPort(port).create();
        if (tracker_conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
            return false;
        }

        if (!tracker_conn->doConnect()) {
            std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            return false;
        }
        std::cerr << "torrent.cpp: registering source port " << sourcePort << '\n';
        // TODO: Stop registering self if possible?
        connections::tracker::send::register_self(tracker_conn, hash, sourcePort);
        message::standard::Header header;
        if (!message::standard::recv(tracker_conn, header)) {
            std::cerr <<print::YELLOW << "[WARN] Could not receive send_exchange request response from tracker: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
            return false;
        }
        return header.formatType == message::standard::OK;
}

// Given a number of tracker servers, constructs a table with all peers these trackers know about
static IPTable compose_peertable(const std::string& hash, const IPTable& trackers, uint16_t sourcePort) {
    std::vector<IPTable> peertables;
    peertables.reserve(trackers.size());

    for (auto it = trackers.cbegin(); it != trackers.cend(); ++it) {
        auto address = it->first;
        auto addr_info = it->second;
        uint16_t port = addr_info.port;

        if (addr_info.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection for non-TCP transport type: '" << addr_info.type.t_type << '\'' << print::CLEAR << '\n';
            continue;
        }

        // TODO: Stop registering self if possible?
        if (!tmp_send_register(addr_info.type, address, port, hash, sourcePort)) {
            std::cerr << "Could not register at a tracker\n";
            continue;
        }

        auto tracker_conn = TCPClientConnection::Factory::from(addr_info.type.n_type).withAddress(address).withDestinationPort(port).create();
        if (tracker_conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
            continue;
        }

        if (!tracker_conn->doConnect()) {
            std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
        if (!connections::tracker::send::receive(tracker_conn, hash)) {
            std::cerr<<"Could not send RECEIVE request to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
        IPTable table;
        std::string recv_hash;
        if (!connections::tracker::recv::receive(tracker_conn, table, recv_hash)) {
            std::cerr<<"Could not receive RECEIVE response from tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
        if (recv_hash != hash) {
            std::cerr << print::RED << "[ERROR] Received peertable had hash mismatch. (Ours=" << hash << ", recvd=" << recv_hash << ')' << print::CLEAR << '\n';
            continue;
        }
        std::cerr<<"Received a peertable from tracker ";tracker_conn->print(std::cerr);std::cerr<<". It has "<< table.size() << " peers\n";
        for (auto it = table.cbegin(); it != table.cend(); ++it)
            std::cerr << it->second.ip << ':' << it->second.port << '\n';
    
        peertables.push_back(table);
    }
    IPTable maintable;
    for (const auto& t : peertables)
        maintable.merge(t);

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
    auto torrent_hash = session.get_metadata().content_hash;
    // unsigned random_start = 0; 
    //TODO @Mariska: 
    // Need something to balance load between peers. Now all peers will probably connect to same others first.
    // Maybe make a random generator in util to generate some random number in a given range?
        // Initialize the random generator with something that is different for every peer... Hostname string? Hostname+ip?
        // Note that initializing with time is maybe/probably not good enough, so only do that if the above option won't work
    // Using iterator to pick a random starting point is inefficient-ish, but maybe the only choice:
        // https://stackoverflow.com/questions/15425442/

    for (auto it = options.cbegin(); it != options.cend() && session.peers_amount() < needed_peers; ++it) {
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
        if (!connections::peer::send::join(conn, sourcePort, torrent_hash)) {
            std::cerr << print::YELLOW << "[WARN] Could not send send_exchange request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        message::standard::Header header;
        if (!message::standard::recv(conn, header)) {
            std::cerr <<print::YELLOW << "[WARN] Could not receive send_exchange request response from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (header.formatType == message::standard::OK) {
            session.add_peer(conn->get_type(), ip, port);
        } else if (header.formatType == message::standard::REJECT) {
            std::cerr << print::CYAN << "[TEST] We got a REJECT for our send_exchange request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        } else {
            std::cerr <<print::YELLOW << "[WARN] Received non-standard-conforming response ("<<header.formatType<<") from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }
    return session.peers_amount() >= needed_peers;
}

// Send request to peers in our local network
static void requests_send(torrent::Session& session) {
    //TODO: 2 options for sending
    // 1. Send using a timeout, 1 by 1. Pro is that we can use 1 port. Con is that 1-by-1 sending is slow.
    // 2. Same as 1, but using multiple threads. Pro is big performance, con is that we use multiple ports.
    // For now we make 1. Adaption to 2 is simple enough to not be a waste of time.

    while (session.get_registry().size() < peer::defaults::torrent::max_outstanding_requests) { // Let's send a request
        // 1. Pick a fragment to request <- balance probabilities: Pick one for which the amount of sent requests is low/minimal
        // 2. Pick a peer to request from  <- balance load: Pick a peer that we did not request much for yet. Somehow pick one that has the data...
        // 3. Request picked fragment at picked peer

        //TODO: Instead of picking first non-completed fragment, pick a random one?
        // const auto& completed = session.get_fragments_completed();
        // const auto& it = std::find(completed.begin(), completed.end(), false);    
        // if (it == completed.end()) //should never happen, because we call this function only when we actually need some fragments
        //     return;
        // size_t fragment_nr = std::distance(completed.begin(), it); //O(1) because vector-like iterator distance results in subtraction
        size_t fragment_nr = 0;
        const auto fragment_max = session.get_num_fragments();
        const auto& registry = session.get_registry();
        for (; fragment_nr < fragment_max; ++fragment_nr)
            if (!session.fragment_completed(fragment_nr) && !registry.contains(fragment_nr)) // Fragment needed and no outgoing requests for fragment yet
                break;
        if (fragment_nr == fragment_max) // All fragments have been retrieved or have outgoing requests
            return;
        //TODO Reminder: need to call gc of registry once in a while
        //TODO: Pick a peer to request from, and balance load
    }
}


// Handle requests we receive
static bool requests_receive(torrent::Session& session) {
    // TODO: Should make this a separate thread

    // All requests are sent to one specific port.
    // In order to maintain which requests we sent, need a registry mapping fragment to Address.
    // Because we request fragments only once, we should have unique keys.
    // The registry needs a notion of timing: "When was request sent?"
    // This is the only way to check for dead requests, and clean them up.
    // Efficiently finding dead requests is pretty much impossible, unless we find some very smart data structure.

    for (uint8_t x = 0; x < 16; ++x) {
        const auto req_conn = session.get_conn();
        auto connection = req_conn->acceptConnection();
        if (connection == nullptr) {
            if (req_conn->get_state() == Connection::ERROR) {
                std::cerr << "Experienced error when checking for inbound communication: ";req_conn->print(std::cerr); std::cerr << '\n';
            } else {
                // Connection is not used at this time
                // TODO: Sleep for a little bit here,
                // or make connection wait for some timeout instead of non-blocking
            }
            continue;
        } else { // We are dealing with an actual connection
            message::standard::Header standard;
            if (!message::standard::recv(connection, standard)) {
                std::cout << "Unable to peek. System hangup?" << std::endl;
                continue;
            }
            const bool message_type_peer = standard.formatType == message::peer::id;
            const bool message_type_standard = standard.formatType == message::standard::id;
            
            if (message_type_peer) {
                uint8_t* const data = (uint8_t*) malloc(standard.size);
                connection->recvmsg(data, standard.size);
                message::peer::Header* header = (message::peer::Header*) data;
                switch (header->tag) {
                    case message::peer::JOIN: peer::pipeline::join(session, connection, data, standard.size); break;
                    case message::peer::LEAVE: peer::pipeline::leave(session, connection, data, standard.size); break;
                    case message::peer::DATA_REQ: peer::pipeline::data_req(session, connection, data, standard.size); break;
                    case message::peer::DATA_REPLY: peer::pipeline::data_reply(session, connection, data, standard.size); break;
                    default: // We get here when testing or corrupt tag
                        break;
                }
                free(data);
            } else if (message_type_standard) {
                switch (standard.tag) {
                    case message::standard::LOCAL_DISCOVERY: peer::pipeline::local_discovery(session, connection); break;
                    default: // We get here when we receive some other or corrupt tag
                        break;
                }
            } else {
                std::cerr << "Received invalid message! Not a Peer-message nor standard-message. Skipping..." << std::endl;
                continue;
            }
        }
    }
    return true;
}

bool torrent::run(const std::string& torrentfile, const std::string& workpath, uint16_t sourcePort) {
    // 0. Prepare and check output location
    // 1. Load trackerlist from tf
    // 2. Get peertables from trackers
    // 3. Merge peertables
    // 4. Construct torrent session (to maintain received fragments, maintain receive connection etc)
    // 5. Continually send and receive data

    if (!fs::is_dir(workpath) && !fs::mkdir(workpath)) {
        std::cerr << print::RED << "[ERROR] Could not construct path '" << workpath << "'" << print::CLEAR << std::endl;
        return false;
    }
    TorrentFile tf = TorrentFile::from(torrentfile);
    const IPTable& tracker_table = tf.getTrackerTable();

    IPTable table = compose_peertable(tf.getMetadata().content_hash, tracker_table, sourcePort);

    auto session = torrent::Session(tf, TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(sourcePort).withBlocking(false).create(), workpath);
    bool stop = false;

    // Continually send and recv data. TODO:
    // Best approach might be to use 2 threads (1 for send, 1 for recv). For now, sequential is good enough.

    while (!stop) {
        if (session.get_peertable().size() == 0 && !session.download_completed()) {
            // 7. Initialize peer network: send requests to a number of peers to join.
            const unsigned needed_peers = 1;
            // TODO: Need reasonable cap on connected peers. 
            // In reality, cap depends on network bandwidth: Keep accepting, until bandwidth is filled up.
            join_peers(session, table, needed_peers);
            // Even if we don't have any/enough peers, we still must handle requests.
            requests_receive(session);
            continue;
        }
        // Only send requests for file fragments if we are not done already
        if (!session.download_completed())
            requests_send(session);
        requests_receive(session);
    }
    return true;
}