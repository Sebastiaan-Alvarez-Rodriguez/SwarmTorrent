#include <chrono>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <random>
#include <stdexcept>
#include <thread>
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
        std::cerr << print::YELLOW << "[WARN] Register: Could not initialize connection: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
        return false;
    }

    if (!tracker_conn->doConnect()) {
        std::cerr<<"Could not connect to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
        return false;
    }
    connections::tracker::send::register_self(tracker_conn, hash, sourcePort);
    const auto& header = message::standard::recv(tracker_conn);
    return header.tag == message::standard::OK;
}

// Given a number of tracker servers, constructs a table with all peers these trackers know about
static IPTable compose_peertable(peer::torrent::Session& session, bool force_register) {
    const std::string& hash = session.get_metadata().content_hash;
    const IPTable& trackers = session.get_trackertable();
    uint16_t sourcePort = session.registered_port;

    std::vector<IPTable> peertables;
    peertables.reserve(trackers.size());

    for (auto it = trackers.cbegin(); it != trackers.cend(); ++it) {
        const auto& address = *it;

        if (address.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection for non-TCP transport type: '" << address.type.t_type << '\'' << print::CLEAR << '\n';
            continue;
        }

        if (force_register && !send_register(address.type, address.ip, address.port, hash, sourcePort)) {
            std::cerr << "Could not register at a tracker\n";
            continue;
        } else {
            std::cerr << "Registered at remote tracker!\n";
        }

        auto tracker_conn = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
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
        if (!connections::tracker::recv::receive(tracker_conn, table, own_address, sourcePort)) {
            std::cerr<<"Could not recv RECEIVE reply from tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        }

        if (session.get_address().port == 0)
            session.set_address(own_address);
        std::cerr<<"Received a peertable from tracker ";tracker_conn->print(std::cerr);std::cerr<<". It has "<< table.size() << " peers\n";
        for (auto it = table.cbegin(); it != table.cend(); ++it)
            std::cerr << it->ip << ':' << it->port << '\n';

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

// Request any peer in our group to provide us with the peers they know
static void requests_send_local_discovery(peer::torrent::Session& session, rnd::RandomGenerator<size_t>& rand) {
    const auto registry = session.get_peer_registry_copy();
    if (registry.size() == 0) // Cannot discover on an empty registry. First need to join some peers.
        return;

    const size_t peertable_size = session.num_known_peers();
    if (peertable_size >= peer::torrent::defaults::prefered_known_peers_size) // have enough peers
        return;

    size_t diff = peer::torrent::defaults::prefered_known_peers_size - peertable_size;

    IPTable extrapeertable;
    uint16_t tries = 0;
    while (extrapeertable.size() < diff && tries < peer::torrent::defaults::discovery_retries) {
        // 1. Pick a peer to ask more info from
        // 2. Send local discovery request
        // 3. Receive peers
        // 4. Merge with current

        ++tries;
        const auto peer_idx = rand.generate(0, registry.size()-1);
        auto it = registry.cbegin();
        std::advance(it, peer_idx);
        const auto& address = it->first;

        std::cerr << "Sending a LOCAL_DISCOVERY_REQ to " << address.type << ", " << address.ip << ':'<<address.port<<'\n';
        auto connection = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (!connection->doConnect()) {
            session.report_registered_peer(address);
            std::cerr << "Could not connect to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        session.mark_registered_peer(address);
        if (!connections::shared::send::discovery_req(connection, session.get_metadata().content_hash)) {
            std::cerr << "Could not send local_discovery request to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        

        message::standard::Header standard = message::standard::recv(connection);

        uint8_t* const data = (uint8_t*) malloc(standard.size);
        connection->recvmsg(data, standard.size);

        IPTable recvtable;
        std::string recv_hash;
        if (!connections::shared::recv::discovery_reply(data, standard.size, recvtable, recv_hash)) {
            std::cerr << "Could not recv local_discovery reply from fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            free(data);
            continue;
        }
        free(data);
        std::cerr << "Interpreted reply: Received " << recvtable.size() << " peers.\n";
        if (recv_hash != session.get_metadata().content_hash) {
            std::cerr << "Received hash mismatching our own. (Ours="<<session.get_metadata().content_hash<<", theirs="<<recv_hash<<")\n";
            continue;
        }
        extrapeertable.merge(recvtable);
    }
    extrapeertable.remove(session.get_address());
    session.add_peers(extrapeertable);
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
    
    const auto registry = session.get_peer_registry_copy(); // we just use copies instead of locks. The registry is not that large anyway.
    if (registry.size() >= peer::torrent::defaults::prefered_group_size) // our group is large enough
        return;
    size_t diff = peer::torrent::defaults::prefered_group_size - registry.size();


    auto peertable_lock = session.get_peertable_lock_read();
    const auto& peertable = session.get_peertable_unsafe();

    std::unordered_set<Address> addresses;
    for (auto it = peertable.cbegin(); it != peertable.cend() && addresses.size() < diff; ++it) {
        const auto& address = *it;
        if (registry.contains(address)) // skip if we are already connected to this peer
            continue;
        if (address.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Only implemented support for TCP connections, skipping: " << address.type << ": " << address.ip << ':' << address.port << '\n';
            continue;
        }
        addresses.insert(address);
    }

    peertable_lock.unlock();

    for (const auto& address : addresses) {
        auto conn = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (conn->get_state() != ClientConnection::READY) {
            session.report_registered_peer(address);
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!conn->doConnect()) {
            session.report_registered_peer(address);
            std::cerr << "Could not connect to peer ";conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        session.mark_registered_peer(address);
        
        std::cerr << "Sending a JOIN request\n";
        if (!connections::peer::send::join(conn, session.registered_port, torrent_hash, session.get_fragments_completed_copy())) {
            std::cerr << print::YELLOW << "[WARN] Could not send send_exchange request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        message::standard::Header header = message::standard::recv(conn);
        if (header.tag == message::standard::OK) {
            std::cerr << print::CYAN << "[TEST] We got an OK for our JOIN request from peer: " << print::CLEAR; conn->print(std::cerr); std::cerr << '\n';
            std::string recv_hash;
            std::vector<bool> remote_available;

            uint8_t* const data = (uint8_t*) malloc(header.size);
            conn->recvmsg(data, header.size);

            connections::peer::recv::join_reply(data, header.size, recv_hash, remote_available);
            free(data);

            session.register_peer({conn->get_type(), address.ip, address.port}, remote_available);
        } else if (header.tag == message::standard::REJECT) {
            std::cerr << print::CYAN << "[TEST] We got a REJECT for our send_exchange request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        } else {
            std::cerr <<print::YELLOW << "[WARN] Received non-standard-conforming response ("<<header.tag<<") from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }
}

// Send leave requests to a number of peers. 
// We stop when enough leaves have been sent.
static void requests_send_leave(peer::torrent::Session& session) {
    const auto torrent_hash = session.get_metadata().content_hash;

    const auto registry = session.get_peer_registry_copy();
    if (registry.size() <= 4 * peer::torrent::defaults::prefered_group_size) // not too many peers in our group
        return;

    size_t diff = 4 * peer::torrent::defaults::prefered_group_size - registry.size();

    std::unordered_set<Address> addresses;
    for (auto it = registry.cbegin(); it != registry.cend() && addresses.size() < diff; ++it)
        addresses.insert(it->first);


    for (const auto& address : addresses) {
        // 1. Pick a peer of our group
        // 2. Send LEAVE
        // 3. Remove peer from registry

        if (address.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Only implemented support for TCP connections, skipping: " << address.type << ": " << address.ip << ':' << address.port << '\n';
            continue;
        }

        auto conn = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (conn->get_state() != ClientConnection::READY) {
            session.mark_registered_peer(address);
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!conn->doConnect()) {
            session.mark_registered_peer(address);
            std::cerr << "Could not connect to peer ";conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        session.report_registered_peer(address);
        if (!connections::peer::send::leave(conn, torrent_hash, session.registered_port)) {
            std::cerr << print::YELLOW << "[WARN] Could not send leave request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        session.deregister_peer(address);
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
static void requests_send_data_req(peer::torrent::Session& session, rnd::RandomGenerator<size_t>& rand) {
    // 1. Pick a fragment to request
    // 2. Pick a peer to request from
    // 3. Request picked fragment at picked peer

    if (session.download_completed()) // We are not asking for data if we already have all data
        return;
    const auto peer_registry = session.get_peer_registry_copy();
    if (peer_registry.size() == 0) // We have nobody to request data from
        return;

    const size_t request_registry_size = session.num_requests();
    if (request_registry_size >= peer::torrent::defaults::outgoing_requests) // already have max connections out
        return;

    const size_t diff = peer::torrent::defaults::outgoing_requests - request_registry_size;


    // pick fragments
    std::unordered_set<size_t> picked_fragments;
    const auto completed_vector = session.get_fragments_completed_copy();
    for (size_t x = 0; x < diff; ++x) {
        const auto fragment_nr = rnd::random_from(rand, completed_vector, false);
        if (fragment_nr >= session.num_fragments) // We get here only if all fragments are completed
            break; // No need to ask for fragment data if we already have all
        picked_fragments.insert(fragment_nr);
    }

    std::unordered_map<size_t, Address> addresses;
    // pick an address for each fragment 
    for (const auto& fragment_nr : picked_fragments) {
        const auto containmentvector = peer_registry.get_peers_for(fragment_nr);

        if (containmentvector.size() == 0) { // If nobody owns the fragment we seek, pick a random peer
            const auto peer_idx = rand.generate(0, peer_registry.size()-1);
            auto it = peer_registry.cbegin();
            std::advance(it, peer_idx);
            addresses.insert({fragment_nr, it->first});
        } else { // Randomly pick a peer that owns the fragment we want 
            const auto peer_idx = rand.generate(0, containmentvector.size()-1);
            addresses.insert({fragment_nr, containmentvector[peer_idx]});
        }
    }

    // send a request, process the response
    for (const auto& [key, val] : addresses) {
        const auto& address = val;
        const auto& fragment_nr = key;
        auto connection = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (!connection->doConnect()) {
            std::cerr << "Could not connect to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            session.report_registered_peer(address);
            continue;
        }
        session.mark_registered_peer(address);
        if (!connections::peer::send::data_req(connection, session.registered_port, fragment_nr)) {
            std::cerr << "Could not send data request to fellow peer ";connection->print(std::cerr); std::cerr << '\n';
            continue;
        }
        

        message::standard::Header standard = message::standard::recv(connection);
        // std::cout << "Unable to peek. System hangup?" << std::endl;
        // continue;

        switch(standard.tag) {
            case message::standard::OK: session.register_request(fragment_nr, address); break; // All is good, message in progress.
            case message::standard::REJECT: { // Remote does not have that data available
                uint8_t* const data = (uint8_t*) malloc(standard.size);
                connection->recvmsg(data, standard.size);

                uint8_t* reader = data;
                size_t remaining_size = standard.size - message::peer::bytesize();
                std::vector<bool> completed_update(remaining_size);
                for (size_t x = 0; x < remaining_size; ++x) {
                    completed_update[x] = *(bool*) reader;
                    reader += sizeof(bool);
                }
                session.update_registered_peer_fragments(address, std::move(completed_update));
                break;
            }
            case message::standard::ERROR: // Somehow, we are no longer part of the group of remote peer
                session.deregister_peer(address);
                break;
            default:
                std::cerr << "Received non-standard-conforming message from remote peer: "; connection->print(std::cerr); std::cerr << '\n';
        }
    }
}

/**
 * Sends/receives availability requests for data. With each request, we also send our own availability.
 * We wait for a response, which can be either:
 * - OK for success.
 * - ERROR to let us know we are no longer in their group.
 *
 * In case of OK, the message body contains the byte array telling us which fragments it owns.
 * '''Note:''' Due to amount of communication, it is recommended to call this function from a separate thread.
 */
static void requests_send_availability(peer::torrent::Session& session) {
    // 1. For every peer in our group, send an availability request
    // 2. get reply back, store new availability
    const auto torrent_hash = session.get_metadata().content_hash;
    const auto peer_registry = session.get_peer_registry_copy();

    for (auto it = peer_registry.cbegin(); it != peer_registry.cend(); ++it) {
        const auto& address = it->first;

        if (address.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Only implemented support for TCP connections, skipping: " << address.type << ": " << address.ip << ':' << address.port << '\n';
            continue;
        }

        auto conn = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();
        if (conn->get_state() != ClientConnection::READY) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!conn->doConnect()) {
            std::cerr << "Could not connect to peer ";conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        if (!connections::peer::send::availability(conn, session.registered_port, torrent_hash, session.get_fragments_completed_copy())) {
            std::cerr << print::YELLOW << "[WARN] Could not send AVAILABILITY request to peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            continue;
        }
        message::standard::Header header = message::standard::recv(conn);
        // std::cerr <<print::YELLOW << "[WARN] Could not receive AVAILABILITY response from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        // continue;
        if (header.tag == message::standard::OK) {
            std::cerr << print::CYAN << "[TEST] We got an OK for our AVAILABILITY request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            uint8_t* const data = (uint8_t*) malloc(header.size);
            conn->recvmsg(data, header.size);

            std::vector<bool> remote_available;
            connections::peer::recv::availability_reply(data, header.size, remote_available);
            free(data);
            session.update_registered_peer_fragments(address, std::move(remote_available));
        } else if (header.tag == message::standard::ERROR) {
            std::cerr << print::CYAN << "[TEST] We got a ERROR for our AVAILABILITY request from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
            session.deregister_peer(address);
        } else {
            std::cerr <<print::YELLOW << "[WARN] Received non-standard-conforming response ("<<header.tag<<") (for AVAILABILITY request) from peer: " << print::CLEAR; conn->print(std::cerr);std::cerr << '\n';
        }
    }
}

// Send request to peers in our local network
static void requests_send(peer::torrent::Session& session, rnd::RandomGenerator<size_t>& rand) {
    //TODO: 2 options for sending
    // 1. Send using a timeout, 1 by 1. Pro is that we can use 1 port. Con is that 1-by-1 sending is slow.
    // 2. Same as 1, but using multiple threads. Pro is big performance, con is that we use multiple ports.
    // For now we make 1. Adaption to 2 is simple enough to not be a waste of time.
    size_t known_peers = session.num_known_peers();
    if (known_peers == 0) { // We have 0 peers. Ask tracker to provide us peers
        std::this_thread::sleep_for(::peer::torrent::defaults::dead_torrent_poke_time);
        std::cerr << "We now have 0 peers! We must ask the trackers for a peertable\n";
        session.set_peers(compose_peertable(session, false));
        return;
    } else {
        std::cerr << print::RED << "We have now " << known_peers << " known peers.\n" << print::CLEAR;
    }
    // We have at least 1 peer. Get some data!
    // 1. while small peertable -> LOCAL_DISCOVERY_REQ
    // 2. while small jointable -> JOIN
    // 3. while large jointable -> LEAVE
    // 4. while #requests < max -> DATA_REQ

    // requests_send_local_discovery(session, rand);

    const size_t num_registered_peers = session.num_registered_peers();
    if (num_registered_peers < peer::torrent::defaults::prefered_group_size)
        requests_send_join(session);
    else if (num_registered_peers > 4 * peer::torrent::defaults::prefered_group_size)
        requests_send_leave(session);

    requests_send_data_req(session, rand);
}


// Handle requests we receive
static void requests_receive(peer::torrent::Session& session, std::unique_ptr<HostConnection>& hostconnection, FragmentHandler& handler, const std::string logfile, const std::chrono::high_resolution_clock::time_point starttime) {
    auto connection = hostconnection->acceptConnection();
    message::standard::Header standard = message::standard::recv(connection);
    // std::cerr << "Unable to peek. System hangup?" << std::endl;
    // continue;
    const bool message_type_peer = standard.formatType == message::peer::id;
    const bool message_type_standard = standard.formatType == message::standard::id;

    //TODO: Remove after test:
    size_t s = session.num_known_peers();
    std::cerr << "A: In RECV thread, we have "<<s<<" items in our known peers\n"; 


    if (message_type_peer) {
        uint8_t* const data = (uint8_t*) malloc(standard.size);
        connection->recvmsg(data, standard.size);
        message::peer::Header* header = (message::peer::Header*) data;
        switch (header->tag) {
            case message::peer::JOIN: {
                peer::pipeline::join(session, connection, data, standard.size);
                //TODO: Remove after test:
                size_t s = session.num_known_peers();
                std::cerr << "A: In RECV thread, we have "<<s<<" items in our known peers\n";
                break;
            }
            case message::peer::LEAVE: peer::pipeline::leave(session, connection, data, standard.size); break;
            case message::peer::DATA_REQ: peer::pipeline::data_req(session, connection, handler, data, standard.size); break;
            case message::peer::DATA_REPLY: {
                peer::pipeline::data_reply(session, connection, handler, data, standard.size);
                // If we received all fragments, log time
                if (session.download_completed()) {
                    auto t2 = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-starttime).count();
                    std::ofstream logger(logfile);
                    logger << duration; 
                    logger.close();
                }
                break;
            }
            case message::peer::INQUIRE: message::standard::send(connection, message::standard::OK); break;
            case message::peer::AVAILABILITY: peer::pipeline::availability(session, connection, data, standard.size); break;
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
        return;
    }
}

bool torrent::run(const std::string& torrentfile, const std::string& workpath, uint16_t sourcePort, bool force_register, const std::string& logfile) {
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

    auto hostconnection = TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(sourcePort).create();

    peer::torrent::Session session(tf, workpath, sourcePort);
    session.set_peers(compose_peertable(session, force_register));
    
    std::cerr << "Before booting receivethread: " << session.get_address().ip<< ':' << session.get_address().port << '\n';
    bool stop = false;
    bool availability_stop = false;
    bool receive_stop = false;
    bool gc_stop = false;

    // std::thread availabilitythread;
    // const bool use_availabilitythread = session.download_completed();
    // if (use_availabilitythread) // Periodically send our availability, and ask for availability of others
    //     availabilitythread = std::thread([&session, &availability_stop]() {
    //         while (!availability_stop && !session.download_completed()) {
    //             requests_send_availability(session);
    //             std::this_thread::sleep_for(::peer::torrent::defaults::availability_update_time);
    //         }
    //     });
    std::thread receivethread([&session, &receive_stop, &logfile](auto&& hostconnection) {
        auto starttime = std::chrono::high_resolution_clock::now();
        FragmentHandler fragmentHandler(session.metadata, session.workpath + session.metadata.name);
        while (!receive_stop) {
            requests_receive(session, hostconnection, fragmentHandler, logfile, starttime);
            std::this_thread::sleep_for(std::chrono::milliseconds(6000));
        }
    }, std::move(hostconnection));
    // std::thread gcthread([&session, &gc_stop]() {
    //     while (!gc_stop) {
    //         session.peer_registry_gc();
    //         session.request_registry_gc();
    //         std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    //     }
    // });


    // Simple random number generator to use during this session.
    // Initialized such that different peers generate different numbers
    rnd::RandomGenerator<size_t> rand(std::move(std::random_device()));

    std::cerr << "Start main program loop.\n";
    while (!stop) {
        requests_send(session, rand);
        std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    }

    // availability_stop = true;
    // if (use_availabilitythread)
    //     availabilitythread.join();

    receive_stop = true;
    receivethread.join();
    
    // gc_stop = true;
    // gcthread.join();
    return true;
}