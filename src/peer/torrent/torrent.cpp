#include <cstdint>
#include <iostream>
#include <random>
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
#include "shared/util/fs/fs.h"
#include "shared/util/print.h"
#include "shared/util/random/random.h"
#include "shared/util/random/randomGenerator.h"
#include "torrent.h"

static bool send_register(ConnectionType type, std::string address, uint16_t port, std::string hash, uint16_t sourcePort) {
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
static IPTable compose_peertable(const std::string& hash, const IPTable& trackers, uint16_t sourcePort, bool force_register) {
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

        if (force_register && !send_register(addr_info.type, address, port, hash, sourcePort)) {
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
        Address own_address;
        if (!connections::tracker::recv::receive(tracker_conn, hash, table, own_address, sourcePort)) {
            std::cerr<<"Could not recv RECEIVE reply from tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }
        std::cerr << "own address: " << own_address.ip << ':' << own_address.port << '\n';
        std::cerr<<"Received a peertable from tracker ";tracker_conn->print(std::cerr);std::cerr<<". It has "<< table.size() << " peers\n";
        for (auto it = table.cbegin(); it != table.cend(); ++it)
            std::cerr << it->second.ip << ':' << it->second.port << '\n';

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
static bool join_peers(torrent::Session& session, unsigned needed_peers) {
    auto torrent_hash = session.get_metadata().content_hash;
    // unsigned random_start = 0; 
    //TODO @Mariska: 
    // Need something to balance load between peers. Now all peers will probably connect to same others first.
    // Maybe make a random generator in util to generate some random number in a given range?
        // Initialize the random generator with something that is different for every peer... Hostname string? Hostname+ip?
        // Note that initializing with time is maybe/probably not good enough, so only do that if the above option won't work

    auto options = session.get_peertable();
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
        if (!connections::peer::send::join(conn, sourcePort, torrent_hash, session.get_fragments_completed())) {
            std::cerr << print::YELLOW << "[WARN] Could not send send_exchange request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        message::standard::Header header;
        if (!message::standard::recv(conn, header)) {
            std::cerr <<print::YELLOW << "[WARN] Could not receive send_exchange request response from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (header.formatType == message::standard::OK) {
            std::cerr << print::CYAN << "[TEST] We got an OK for our send_exchange request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            std::string recv_hash;
            std::vector<bool> remote_available;

            uint8_t* const data = (uint8_t*) malloc(header.size);
            connections::peer::recv::join_reply(data, header.size, recv_hash, remote_available);
            free(data);
            session.register_peer(conn->get_type(), ip, port, remote_available);
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

    // 1. while small peertable -> LOCAL_DISCOVERY_REQ
    // 2. while small jointable -> JOIN
    // 3. while large jointable -> LEAVE
    // 4. while #requests < max -> DATA_REQ
    // 5. while suspected dead in jointable -> INQUIRE

    while (session.get_peer_registry().size() < peer::defaults::torrent::prefered_group_size) { // Let's get a peer to join our network
        // 1. Pick a peer to add to the network. Preferably one with content we do not have in other peers?
        // 2. Send the standard JOIN request. Receive the reply... TODO: timeout, or even beter, get result to pipeline
        // 3. Add to registry
    }
    while (session.get_request_registry().size() < peer::defaults::torrent::max_outstanding_requests) { // Let's send a request
        // 1. Pick a fragment to request
        // 2. Pick a peer to request from
        // 3. Request picked fragment at picked peer

        
        //TODO @Mariska making a random device for every new sending is expensive. Move into session
        std::random_device random_gen;
        auto gen = rnd::RandomGenerator<size_t>(random_gen);

        const auto fragment_nr = rnd::random_from(gen, session.get_fragments_completed(), false);
        if (fragment_nr >= session.get_num_fragments()) // We get here if all fragments are completed
            return; // No need to ask for fragment data if we already have all

        //TODO Reminder: need to call gc of registry once in a while

        const auto peer_idx = gen.generate(0, session.get_peertable().size());
        auto it = session.get_peertable().cbegin();
        std::advance(it, peer_idx);
        const auto address = it->second;
        auto connection = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (!connection->doConnect()) {
            std::cerr << "Could not connect to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        if (!connections::peer::send::data_req(connection, fragment_nr)) {
            std::cerr << "Could not send data request to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        session.register_request(fragment_nr, address);
    }
}


// Handle requests we receive
static bool requests_receive(torrent::Session& session) {
    // TODO: Should make this a separate thread

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
            session.mark_peer(connection->getAddress()); // Marks peer as seen. TODO: remove timestamp to hold timestamp for availability request updates?

            message::standard::Header standard;
            if (!message::standard::recv(connection, standard)) {
                std::cerr << "Unable to peek. System hangup?" << std::endl;
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
                        std::cerr << "Received an unimplemented peer tag: " << header->tag << '\n';
                        break;
                }
                free(data);
            } else if (message_type_standard) {
                uint8_t* const data = (uint8_t*) malloc(standard.size);
                connection->recvmsg(data, standard.size);
                switch (standard.tag) {
                    case message::standard::LOCAL_DISCOVERY_REQ: peer::pipeline::local_discovery(session, connection, data, standard.size); break;
                    default: // We get here when we receive some other or corrupt tag
                        std::cerr << "Received an unimplemented standard tag: " << standard.tag << '\n';
                        break;
                }
            } else {
                std::cerr << "Received invalid message! Not a Peer-message, nor a standard-message. Skipping..." << std::endl;
                continue;
            }
        }
    }
    return true;
}

bool torrent::run(const std::string& torrentfile, const std::string& workpath, uint16_t sourcePort, bool force_register) {
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


    auto session = torrent::Session(tf, TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(sourcePort).create(), workpath);
    session.set_peers(compose_peertable(tf.getMetadata().content_hash, tracker_table, sourcePort, force_register));
    bool stop = false;

    // Continually send and recv data. TODO:
    // Best approach might be to use 2 threads (1 for send, 1 for recv). For now, sequential is good enough.

    while (!stop) {
        if (session.get_peertable().size() == 0 && !session.download_completed()) {
            // 7. Initialize peer network: send requests to a number of peers to join.
            const unsigned needed_peers = 1;
            // TODO: Need reasonable cap on connected peers. 
            // In reality, cap depends on network bandwidth: Keep accepting, until bandwidth is filled up.
            join_peers(session, needed_peers);
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