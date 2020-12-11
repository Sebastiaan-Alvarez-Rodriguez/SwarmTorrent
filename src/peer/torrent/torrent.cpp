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
        connections::tracker::register_self(tracker_conn, hash, sourcePort);
        message::standard::Header header;
        if (!message::standard::recv(tracker_conn, header)) {
            std::cerr <<print::YELLOW << "[WARN] Could not receive send_exchange request response from tracker: " << print::CLEAR; tracker_conn->print(std::cerr);std::cerr << '\n';
            return false;
        }
        return header.formatType == message::standard::OK;
}


static IPTable compose_peertable(const std::string& hash, const IPTable& trackers, uint16_t sourcePort) {
    std::vector<IPTable> peertables;
    peertables.reserve(trackers.size());

    // 2. Connect to trackers in list
    for (auto it = trackers.cbegin(); it != trackers.cend(); ++it) {
        auto address = it->first;
        auto addr_info = it->second;
        uint16_t port = addr_info.port;

        if (addr_info.type.t_type != TransportType::TCP) {
            std::cerr << print::YELLOW << "[WARN] Could not initialize connection for non-TCP transport type: '" << addr_info.type.t_type << '\'' << print::CLEAR << '\n';
            continue;
        }

        // // TODO: Stop registering self if possible?
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

        // 3. Request trackers to provide peertables
        // 4. Receive peertables
        IPTable table;
        if (!connections::tracker::receive(tracker_conn, hash, table)) {
            std::cerr<<"Could not send RECEIVE request to tracker ";tracker_conn->print(std::cerr);std::cerr<<'\n';
            continue;
        } else {
            std::cerr<<"Received a peertable from tracker ";tracker_conn->print(std::cerr);std::cerr<<". It has "<< table.size() << " peers\n";
            for (auto it = table.cbegin(); it != table.cend(); ++it)
                std::cerr << it->second.ip << ':' << it->second.port << '\n';
        }
        peertables.push_back(table);
    }
    // 5. Unify peertables
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

static bool requests_send(torrent::Session& session) {
    //TODO: Stub
    // 9. Send requests to get data
    //TODO: 2 options for sending
    // 1. Send using a timeout, 1 by 1. Pro is that we can use 1 port. Con is that 1-by-1 sending is slow.
    // 2. Same as 1, but using multiple threads. Pro is big performance, con is that we use multiple ports.
    // For now we make 1. Adaption to 2 is simple enough to not be a waste of time.

    return true || session.download_completed();
}

static void handle_join(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    //TODO: For now always accept join requests. In the future, add reasonable limit on joined peers
    uint16_t req_port;
    std::string hash;
    connections::peer::recv::join(data, size, hash, req_port);
    std::cerr << "Got an JOIN (hash=" << hash << ", req_port=" << req_port << ")\n";
    if (session.get_metadata().content_hash != hash) { // Torrent mismatch, Reject
        std::cerr << "Above hash mismatched with our own (" << session.get_metadata().content_hash <<"), rejected.\n";
        message::standard::send(connection, message::standard::REJECT);
        return;
    }
    session.add_peer(connection->get_type(), connection->getAddress(), req_port);
    message::standard::send(connection, message::standard::OK);
}

static void handle_leave(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    uint16_t req_port;
    std::string hash;
    connections::peer::recv::leave(data, size, hash, req_port);
    std::cerr << "Got a lEAVE (hash=" << hash << ", req_port=" << req_port << ")\n";
    if (session.get_metadata().content_hash != hash) // Torrent mismatch, ignore
        return;
    session.remove_peer(connection->getAddress(), req_port);
}

static void handle_data_req(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    //STOPSHIP 2020-12-11:
    // 1. Check if peer ip is in session
    // 2. Read data from disk with fragmentHandler
    // 3. Use fast sending method to reply data.
    if (!session.knows_peer(connection->getAddress())) { //Reject data requests from unknown entities
        message::standard::send(connection, message::standard::REJECT);
        return;
    }

}

static bool requests_receive(torrent::Session& session) {
    //TODO: 2 options for handling receiving fragments
    // 1. Use connection used by the request to send back the data. Pro: easy to make. Con: Makes sending and receiving non-separable
    // 2. Send received fragments to a specific port. Pro: Separates sending and receiving. Con: We have to somehow remember which requests we sent out.
    //    In order to maintain which requests we sent, need a registry mapping fragment to Address.
    //    Because we request fragments only once, we should have unique keys.
    //    The registry needs a notion of timing: "When was request sent?"
    //    This is the only way to check for dead requests, and clean them up.
    //    Efficiently finding dead requests is pretty much impossible, unless we find some very smart data structure.

    // 10. Handle incoming connections
    //TODO: Maybe should use polling to accept clients?
    while (true) {
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
                case message::peer::JOIN: handle_join(session, connection, data, standard.size); break;
                case message::peer::LEAVE: handle_leave(session, connection, data, standard.size); break;
                case message::peer::DATA_REQ: handle_data_req(session, connection, data, standard.size); break;
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
}

bool torrent::run(const std::string& torrentfile, const std::string& workpath, uint16_t sourcePort) {
    // 0. Prepare and check output location
    if (!fs::is_dir(workpath) && !fs::mkdir(workpath)) {
        std::cerr << print::RED << "[ERROR] Could not construct path '" << workpath << "'" << print::CLEAR << std::endl;
        return false;
    }
    // 1. Load trackerlist from tf
    TorrentFile tf = TorrentFile::from(torrentfile);
    const IPTable& tracker_table = tf.getTrackerTable();

    IPTable table = compose_peertable(tf.getMetadata().content_hash, tracker_table, sourcePort);

    // 6. Construct torrent session (to maintain received fragments)
    auto session = torrent::Session(tf, TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(sourcePort).withBlocking(false).create(), workpath);

    bool stop = false;

    // Continually send and recv data.
    // Best approach might be to use 2 threads (1 for send, 1 for recv). For now, sequential is good enough.
    // We should use the same connection (and thus port) for requesting and receiving data, because otherwise, we would have to disconnect and reconnect a lot.

    const uint32_t max_outstanding_fragment_requests = 10;
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