#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
#include <future>
#include <chrono>

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
        std::cerr << print::YELLOW << "[WARN] Register: Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
        return false;
    }

    if (!tracker_conn->doConnect()) {
        std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
        return false;
    }
    std::cerr << "torrent.cpp: registering source port " << sourcePort << '\n';
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
            std::cerr << print::YELLOW << "[WARN] Compose: Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
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

// Request any peer in our peertable to provide us with the peers they know
static void requests_send_local_discovery(peer::torrent::Session& session) {
    while (session.get_peertable().size() < peer::torrent::defaults::prefered_known_peers_size) {
        // 1. Pick a peer to ask more info from
        // 2. Send local discovery request
        // 3. Receive peers
        // 4. Merge with current
        const auto registry = session.get_peer_registry();
        if (registry.size() == 0) // Cannot discover on an empty registry. First need to join some peers.
            return;
        const auto peer_idx = session.rand.generate(0, session.get_peer_registry().size());
        auto it = registry.cbegin();
        std::advance(it, peer_idx);
        const auto& address = it->second.address;
        auto connection = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (!connection->doConnect()) {
            std::cerr << "Could not connect to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        if (!connections::shared::send::discovery_req(connection, session.get_metadata().content_hash)) {
            std::cerr << "Could not send local_discovery request to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }

        message::standard::Header standard;
        if (!message::standard::recv(connection, standard)) {
            std::cout << "Unable to peek. System hangup?" << std::endl;
            continue;
        }
        uint8_t* const data = (uint8_t*) malloc(standard.size);
        connection->recvmsg(data, standard.size);

        IPTable extrapeertable;
        std::string recv_hash;
        if (!connections::shared::recv::discovery_reply(data, standard.size, extrapeertable, recv_hash)) {
            std::cerr << "Could not recv local_discovery reply from fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            free(data);
            continue;
        }
        if (recv_hash != session.get_metadata().content_hash) {
            std::cerr << "Received hash mismatching our own. (Ours="<<session.get_metadata().content_hash<<", theirs="<<recv_hash<<")\n";
            free(data);
            continue;
        }
        session.add_peers(extrapeertable);
        free(data);
    }
}

// Send join requests to a number of peers. 
static void requests_send_join(peer::torrent::Session& session) {
    auto torrent_hash = session.get_metadata().content_hash;
    //TODO @Mariska: 
    // Need something to balance load between peers. Now all peers will probably connect to same others first.
    // Elements are unordered, but do the same values make the same hashmap? If not, no problems...
    // Maybe make a random generator in util to generate some random number in a given range?
        // Initialize the random generator with something that is different for every peer... Hostname string? Hostname+ip?
        // Note that initializing with time is maybe/probably not good enough, so only do that if the above option won't work

    auto options = session.get_peertable();
    for (auto it = options.cbegin(); it != options.cend() && session.peers_amount() < peer::torrent::defaults::prefered_group_size; ++it) {
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
            conn->recvmsg(data, header.size);

            connections::peer::recv::join_reply(data, header.size, recv_hash, remote_available);
            free(data);
            session.register_peer(conn->get_type(), ip, port, remote_available);
        } else if (header.formatType == message::standard::REJECT) {
            std::cerr << print::CYAN << "[TEST] We got a REJECT for our send_exchange request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        } else {
            std::cerr <<print::YELLOW << "[WARN] Received non-standard-conforming response ("<<header.formatType<<") from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }
}

// Send leave requests to a number of peers. 
// We stop when enough leaves have been sent.
static void requests_send_leave(peer::torrent::Session& session) {
    auto torrent_hash = session.get_metadata().content_hash;

    auto registry = session.get_peer_registry();
    for (auto it = registry.cbegin(); it != registry.cend() && session.peers_amount() > 4 * peer::torrent::defaults::prefered_group_size; ++it) {
        // 1. Pick a peer of our group
        // 2. Send LEAVE
        // 3. Remove peer from registry

        const auto& ip = it->first;
        uint16_t port = it->second.address.port;
        const auto& type = it->second.address.type;

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
        if (!connections::peer::send::leave(conn, torrent_hash, sourcePort)) {
            std::cerr << print::YELLOW << "[WARN] Could not send leave request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        session.deregister_peer(ip);
    }
}

/**
 * Send requests for data. We wait until the remote gives us a primary request status indication. We can get:
 * - OK for success.
 * - REJECT to let us know remote does not possess requested fragment. It sends back a byte vector indicating owned fragments.
 * - ERROR to let us know we are no longer in their group.
 *
 * In case of OK, the remote peer will send us the fragment to our registered port on the receive thread.
 * That message will have tag DATA_REPLY.
 */
static void requests_send_data_req(peer::torrent::Session& session) {
    if (session.download_completed()) // We are not asking for data if we already have all data
        return;
    const auto peer_registry = session.get_peer_registry();
    if (peer_registry.size() == 0) // We have nobody to request data from
        return;

    while (session.get_request_registry().size() < peer::torrent::defaults::outgoing_requests) { // Let's send a request
        // 1. Pick a fragment to request
        // 2. Pick a peer to request from
        // 3. Request picked fragment at picked peer

        const auto fragment_nr = rnd::random_from(session.rand, session.get_fragments_completed(), false);
        if (fragment_nr >= session.get_num_fragments()) // We get here only if all fragments are completed
            return; // No need to ask for fragment data if we already have all

        const auto containmentvector = peer_registry.get_peers_for(fragment_nr);

        Address address;
        if (containmentvector.size() == 0) { // If nobody owns the fragment we seek, pick a random peer
            const auto peer_idx = session.rand.generate(0, peer_registry.size());
            auto it = peer_registry.cbegin();
            std::advance(it, peer_idx);
            address = it->second.address;
        } else { // Randomly pick a peer that owns the fragment we want 
            const auto peer_idx = session.rand.generate(0, containmentvector.size());
            address = containmentvector[peer_idx];
        }

        auto connection = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (!connection->doConnect()) {
            std::cerr << "Could not connect to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        if (!connections::peer::send::data_req(connection, fragment_nr)) {
            std::cerr << "Could not send data request to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }

        message::standard::Header standard;
        if (!message::standard::recv(connection, standard)) {
            std::cout << "Unable to peek. System hangup?" << std::endl;
            continue;
        }

        switch(standard.tag) {
            case message::standard::OK: session.register_request(fragment_nr, address); break; // All is good, message in progress.
            case message::standard::REJECT: { // Remote does not have that data available
                uint8_t* const data = (uint8_t*) malloc(standard.size);
                connection->recvmsg(data, standard.size);

                uint8_t* reader = data;
                size_t remaining_size = standard.size - sizeof(message::peer::Header);
                std::vector<bool> completed_update(remaining_size);
                for (size_t x = 0; x < remaining_size; ++x) {
                    completed_update[x] = *(bool*) reader;
                    reader += sizeof(bool);
                }
                session.update_registered_peer_fragments(address.ip, std::move(completed_update));
                break;
            }
            case message::standard::ERROR: // Somehow, we are no longer part of the group of remote peer
                session.deregister_peer(address.ip);
                break;
            default:
                std::cerr << "Received non-standard-conforming message from remote peer: "; connection->print(std::cerr); std::cerr << '\n';
        }
    }
}

/**
 * Sends availability requests for data. With each request, we also send our own availability.
 * We wait for a response, which can be either:
 * - OK for success.
 * - ERROR to let us know we are no longer in their group.
 *
 * In case of OK, the message body contains the byte array telling us which fragments it owns.
 * '''Note:''' Due to amount of communication, it is recommended to call this function from a separate thread.
 */
// TODO: To increase efficiency but reduce accuracy, could cache request message and send the cached version multiple times.
static void requests_send_availability(peer::torrent::Session& session) {
    // 1. For every peer in our group, send an availability request
    // 2. get reply back, store new availability
    auto torrent_hash = session.get_metadata().content_hash;
    auto peers = session.get_peer_registry();
    for (auto it = peers.cbegin(); it != peers.cend(); ++it) {
        const auto& ip = it->first;
        uint16_t port = it->second.address.port;
        const auto& type = it->second.address.type;

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
        if (!connections::peer::send::availability(conn, torrent_hash, session.get_fragments_completed())) {
            std::cerr << print::YELLOW << "[WARN] Could not send AVAILABILITY request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        message::standard::Header header;
        if (!message::standard::recv(conn, header)) {
            std::cerr <<print::YELLOW << "[WARN] Could not receive AVAILABILITY response from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (header.formatType == message::standard::OK) {
            std::cerr << print::CYAN << "[TEST] We got an OK for our AVAILABILITY request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            uint8_t* const data = (uint8_t*) malloc(header.size);
            conn->recvmsg(data, header.size);

            std::vector<bool> remote_available;
            connections::peer::recv::availability_reply(data, header.size, remote_available);
            free(data);
            session.update_registered_peer_fragments(ip, std::move(remote_available));
        } else if (header.formatType == message::standard::ERROR) {
            std::cerr << print::CYAN << "[TEST] We got a ERROR for our AVAILABILITY request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            session.deregister_peer(ip);
        } else {
            std::cerr <<print::YELLOW << "[WARN] Received non-standard-conforming response ("<<header.formatType<<") (for AVAILABILITY request) from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }

}

// Send request to peers in our local network
static bool requests_send(peer::torrent::Session& session) {
    //TODO: 2 options for sending
    // 1. Send using a timeout, 1 by 1. Pro is that we can use 1 port. Con is that 1-by-1 sending is slow.
    // 2. Same as 1, but using multiple threads. Pro is big performance, con is that we use multiple ports.
    // For now we make 1. Adaption to 2 is simple enough to not be a waste of time.

    if (session.get_peertable().size() == 0) { // We have 0 peers. Ask tracker to provide us peers
        std::this_thread::sleep_for(::peer::torrent::defaults::dead_peer_poke_time);
        session.set_peers(compose_peertable(session.get_metadata().content_hash, session.get_trackertable(), session.get_conn()->getSourcePort(), false));
    } else { // We have at least 1 peer. Get some data!
        // 1. while small peertable -> LOCAL_DISCOVERY_REQ
        // 2. while small jointable -> JOIN
        // 3. while large jointable -> LEAVE
        // 4. while #requests < max -> DATA_REQ

        requests_send_local_discovery(session);

        if (session.peers_amount() < peer::torrent::defaults::prefered_group_size)
            requests_send_join(session);
        else if (session.peers_amount() > 4 * peer::torrent::defaults::prefered_group_size)
            requests_send_leave(session);

        requests_send_data_req(session);
    }
    return true;
}


// Handle requests we receive
static bool requests_receive(peer::torrent::Session* session, bool* stop) {
    while (!*stop) {
        const auto req_conn = session->get_conn();
        auto connection = req_conn->acceptConnection();
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
                case message::peer::JOIN: peer::pipeline::join(*session, connection, data, standard.size); break;
                case message::peer::LEAVE: peer::pipeline::leave(*session, connection, data, standard.size); break;
                case message::peer::DATA_REQ: peer::pipeline::data_req(*session, connection, data, standard.size); break;
                case message::peer::DATA_REPLY: peer::pipeline::data_reply(*session, connection, data, standard.size); break;
                case message::peer::INQUIRE: message::standard::send(connection, message::standard::OK); break;
                case message::peer::AVAILABILITY: peer::pipeline::availability(*session, connection, data, standard.size); break;
                default: // We get here when testing or corrupt tag
                    std::cerr << "Received an unimplemented peer tag: " << header->tag << '\n';
                    break;
            }
            free(data);
        } else if (message_type_standard) {
            uint8_t* const data = (uint8_t*) malloc(standard.size);
            connection->recvmsg(data, standard.size);
            switch (standard.tag) {
                case message::standard::LOCAL_DISCOVERY_REQ: peer::pipeline::local_discovery(*session, connection, data, standard.size); break;
                default: // We get here when we receive some other or corrupt tag
                    std::cerr << "Received an unimplemented standard tag: " << standard.tag << '\n';
                    break;
            }
        } else {
            std::cerr << "Received invalid message! Not a Peer-message, nor a standard-message. Skipping..." << std::endl;
            continue;
        }
    }

    return true;
}

static void call_gc(peer::torrent::Session* session, bool* stop) {
    while (!*stop) {
        session->peer_registry_gc();
        session->request_registry_gc();

        std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    } 
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

    auto session = peer::torrent::Session(tf, TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(sourcePort).create(), workpath);
    session.set_peers(compose_peertable(tf.getMetadata().content_hash, tracker_table, sourcePort, force_register));
    bool stop = false;
    bool availability_stop = false;
    bool receive_stop = false;
    bool gc_stop = false;


    std::future<void> availability(std::async([&session, &availability_stop]() {
        while (!availability_stop) {
            requests_send_availability(session);
            std::this_thread::sleep_for(::peer::torrent::defaults::availability_update_time);
        }
    }));
    std::future<bool> result(std::async(requests_receive, &session, &receive_stop));
    std::future<void> gc(std::async(call_gc, &session, &gc_stop));
    while (!stop) 
        stop = !requests_send(session);

    receive_stop = true;
    gc_stop = true;
    return true;
}