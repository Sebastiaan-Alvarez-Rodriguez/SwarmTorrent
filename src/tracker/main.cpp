#include <chrono>
#include <cstring>
#include <algorithm>
#include <future>
#include <iostream>
#include <tclap/CmdLine.h>
#include <set>
#include <stdexcept>

#include "shared/connection/impl/TCP/TCPConnection.h"

#include "shared/connection/message/message.h"
#include "shared/connection/protocol/connections.h"
#include "shared/connection/message/tracker/message.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/util/print.h"
#include "tracker/session/session.h"
#include "tracker/defaults.h"

// Usage of std async: https://kholdstare.github.io/technical/2012/12/18/perfect-forwarding-to-async-1.html

/**
 * Prepares a request, by allocating a buffer for given datasize, 
 * starting with a message::standard::Header of given type.
 * 
 * '''Note:''' size of a message::standard::Header is automatically appended to the datasize. Also: caller has to free returned buffer.
 * @return pointer to start of buffer.
 */
static inline uint8_t* prepare_standard_message(size_t datasize, message::standard::Tag tag) {
    uint8_t* const data = (uint8_t*) malloc(message::standard::bytesize()+datasize);
    message::standard::from(datasize, tag).write(data);
    return data;
}

static void handle_discovery(Session* session, bool* stop) {
    std::cout << "Discovery thread started\n";
    while (!(*stop)) {
        const auto& registry = session->get_registry();
        IPTable peertable;

        for (auto it = registry.cbegin(); it != registry.cend(); ++it) {
            const auto& hash = it->first;
            const auto time_diff = std::chrono::steady_clock::now() - it->second.timestamp;
            const auto& table = it->second.table;
            const bool do_discover = 
                (table.size() < tracker::torrent::defaults::fast_update_size && time_diff > tracker::torrent::defaults::fast_update_time)
                || (table.size() < tracker::torrent::defaults::medium_update_size && time_diff > tracker::torrent::defaults::medium_update_time)
                || (table.size() > tracker::torrent::defaults::medium_update_size && time_diff > tracker::torrent::defaults::slow_update_time);
            if (do_discover) {
                // 1. Pick a number of peers to discover from
                // 2. Send discovery requests
                // 3. Receive discovery requests, set new table


                const auto num_peers = std::min(table.size() < tracker::torrent::defaults::fast_update_size ? tracker::torrent::defaults::fast_update_pool_size :
                    (table.size() < tracker::torrent::defaults::medium_update_size ? tracker::torrent::defaults::medium_update_pool_size : tracker::torrent::defaults::slow_update_pool_size), table.size());

                std::set<size_t> used_peers;
                if (num_peers == table.size())
                    for (size_t x = 0; x < table.size(); ++x)
                        used_peers.insert(x);
                else
                    while (used_peers.size() < num_peers)
                        used_peers.insert(session->rand.generate(0, table.size()));

                for (const auto x : used_peers) {
                    auto table_it = it->second.table.cbegin();
                    std::advance(table_it, x);
                    const auto address = *table_it;
                    auto connection = TCPClientConnection::Factory::from(address.type.n_type).withAddress(address.ip).withDestinationPort(address.port).create();

                    if (!connection->doConnect()) {
                        std::cerr << print::RED << "[ERROR] Could not connect to remote peer!" << print::CLEAR << std::endl;
                        continue;
                    }

                    connections::shared::send::discovery_req(connection, address.ip);

                    message::standard::Header standard = message::standard::recv(connection);
                    // std::cout << "Unable to peek. System hangup?" << std::endl;
                    // continue;
                

                    if (standard.formatType != message::tracker::id) {
                        std::cerr << "Received invalid message! Not a Tracker-message. Skipping..." << std::endl;
                        continue;
                    }
                    uint8_t* data = (uint8_t*) malloc(standard.size);
                    connection->recvmsg(data, standard.size);
                    IPTable tmptable;
                    std::string recv_hash;
                    connections::shared::recv::discovery_reply(data, standard.size, tmptable, recv_hash);
                    if (recv_hash != hash) {
                        std::cerr << "LOCAL_DISCOVERY hash mismatch (ours=" << hash << ", theirs=" << recv_hash << ")\n";
                        continue;
                    }
                    peertable.merge(tmptable);
                    free(data);
                }
                session->set_table(hash, std::move(peertable));
            }
        }
        std::this_thread::sleep_for(tracker::torrent::defaults::discovery_tick_time);
    }
}

static void handle_make_torrent(Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    std::string hash((char*) msg+message::tracker::bytesize(), bufsize-message::tracker::bytesize());
    std::cout << "Got a Make Torrent request (hash=" << hash << ")" << std::endl;

    IPTable table;
    if (!session.get_registry().get_table(hash, table)) { // Table does not exist yet, insert new table.
        session.create_table(hash);
        std::cout << "We have " << session.get_registry().size() << " tables (added 1 table)\n";
    }
    message::standard::send(client_conn, message::standard::OK);
}

static void handle_receive(const Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    std::string hash((char*) msg+message::tracker::bytesize(), bufsize-message::tracker::bytesize());
    std::cout << "Got a Receive request (hash=" << hash << ")" << std::endl;

    IPTable table;
    if (!session.get_registry().get_table(hash, table)) { //No table for hash found, return error
        message::standard::send(client_conn, message::standard::ERROR);
        return;
    }

    size_t table_size = table.size() * Address::size();
    //Message contains: header, peeraddress (Address), table 
    size_t msg_size = Address::size()+table_size;
    uint8_t* const data = prepare_standard_message(msg_size, message::standard::OK);
    uint8_t* writer = data + message::standard::bytesize();

    //Write peeraddress
    Address a(client_conn->get_type(), client_conn->getAddress(), client_conn->getDestinationPort());
    writer = a.write_buffer(writer);

    //Write table
    for (auto it = table.cbegin(); it != table.cend(); ++it)
        writer = it->write_buffer(writer);

    if (!client_conn->sendmsg(data, message::standard::bytesize()+msg_size)) {
        std::cerr << "Had problems sending table back to peer "; client_conn->print(std::cerr); std::cerr << '\n';
        free(data);
        return;
    }

    std::cerr << "Sent table containing " << table.size() << " entries:\n";
    for (auto it = table.cbegin(); it != table.cend(); ++it) {
        std::cerr << it->ip << ':' << it->port << '\n';
    }
    free(data);
}

// Handle REGISTER requests
// peermessage (tag REGISTER)
// hash (string)
// port to register (uint16_t)
static void handle_register(Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    const auto hashlength = bufsize-message::tracker::bytesize()-sizeof(uint16_t);
    std::string hash((char*) msg+message::tracker::bytesize(), hashlength);
    uint16_t port_to_register = *(uint16_t*)(msg+message::tracker::bytesize()+hashlength);
    std::cout << "Got a Register request (hash=" << hash << ", port="<< port_to_register<<")" << std::endl;
    
    const auto& registry = session.get_registry();
    if (!registry.has_table(hash)) { // Table does not exist yet, insert new table.
        std::cerr << "Could not find table for hash " << hash << "\n";
        message::standard::send(client_conn, message::standard::ERROR);
    } else { // Table already exists. Add this peer to the list.
        if (session.add_peer(hash, {client_conn->get_type(), client_conn->getAddress(), port_to_register})) {
            std::cout << "Table with hash " << hash << " has "<< registry.size() << " entries (added 1 new entry: " << client_conn->getAddress() << ':' << port_to_register << ")\n";
        } else {
            std::cout << "Table with hash " << hash << " has "<< registry.size() << " entries (added 0 new entries)\n";
        }
        message::standard::send(client_conn, message::standard::OK);
    }
}

static bool run(uint16_t port) {
    auto conn = TCPHostConnection::Factory::from(NetType::IPv4).withSourcePort(port).create();
    if (conn->get_state() != ClientConnection::READY) {
        std::cerr << print::RED << "[ERROR] Could not initialize connection" << print::CLEAR << std::endl;
        return false;
    }

    Session session;
    std::cout << "Session initialized\n";
    bool discovery_stop = false;
    std::future<void> result(std::async(handle_discovery, &session, &discovery_stop));
    std::cout << "Discovery thread initialized\n";
    std::cout << "Listening started on port " << port << std::endl;

    while (true) {
        auto client_conn = conn->acceptConnection();
        std::cout << "ClientConnection accepted with address " << client_conn->getAddress() << ':' << client_conn->getDestinationPort() << std::endl;

        message::standard::Header standard = message::standard::recv(client_conn);
        // std::cout << "Unable to peek. System hangup?" << std::endl;
        // continue;

        if (standard.formatType != message::tracker::id) {
            std::cerr << "Received invalid message! Not a Tracker-message. Skipping..." << std::endl;
            continue;
        }
        uint8_t* ptr = (uint8_t*) malloc(standard.size);
        client_conn->recvmsg(ptr, standard.size);
        const auto& header = message::tracker::Header::read(ptr);
        switch (header.tag) {
            case message::tracker::TEST:
                std::cout << "Got a test message" << std::endl;
                message::standard::send(client_conn, message::standard::OK);
                break;
            case message::tracker::MAKE_TORRENT: handle_make_torrent(session, client_conn, ptr, standard.size); break;
            case message::tracker::RECEIVE: handle_receive(session, client_conn, ptr, standard.size); break;
            case message::tracker::REGISTER: handle_register(session, client_conn, ptr, standard.size); break;
            default: std::cout << "Got unknown header tag: " << (uint16_t) header.tag << std::endl; break;
        }
    }
    return 0;
}

int main(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Tracker Run", ' ', "0.1");
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port to which peer connect",true,1042,"PORT", cmd);
    cmd.parse(argc, argv);

    uint16_t port = portArg.getValue();
    if (port < 1000)
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!"<<print::CLEAR<<std::endl;

    run(port);
    return 0;
}