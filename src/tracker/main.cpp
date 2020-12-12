#include <iostream>
#include <tclap/CmdLine.h>
#include <stdexcept>
#include <cstring>

#include "shared/connection/impl/TCP/TCPConnection.h"

#include "shared/connection/message/message.h"
#include "shared/connection/message/tracker/message.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/util/print.h"
#include "tracker/session/session.h"

static void handle_make_torrent(Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    std::string hash((char*)msg+sizeof(message::tracker::Header), (bufsize-sizeof(message::tracker::Header)));
    std::cout << "Got a Make Torrent request (hash=" << hash << ")" << std::endl;

    IPTable table;
    if (!session.get_table(hash, table)) { // Table does not exist yet, insert new table.
        session.add_table(hash, table);
        std::cout << "We have " << session.size() << " tables (added 1 table)\n";
    }
    message::standard::send(client_conn, message::standard::OK);
}

static void handle_receive(const Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    std::string hash((char*)msg+sizeof(message::tracker::Header), (bufsize-sizeof(message::tracker::Header)));
    std::cout << "Got a Receive request (hash=" << hash << ")" << std::endl;

    IPTable table;
    if (!session.get_table(hash, table)) { //No table for hash found, return error
        message::standard::send(client_conn, message::standard::ERROR);
        return;
    }

    size_t table_size = table.size() * Address::size();
    //Message contains: header, peeraddress (Address), table 
    size_t msg_size = sizeof(message::standard::Header)+Address::size()+table_size;
    uint8_t* const table_buffer = (uint8_t*) malloc(msg_size);
    uint8_t* writer = table_buffer;

    //Writing header
    *((message::standard::Header*) writer) = message::standard::from(msg_size-sizeof(message::standard::Header), message::standard::OK);
    writer += sizeof(message::standard::Header);

    //Writing peeraddress
    Address a(client_conn->get_type(), client_conn->getAddress(), client_conn->getDestinationPort());
    writer = a.write_buffer(writer);

    //Writing table
    for (auto it = table.cbegin(); it != table.cend(); ++it)
        writer = it->second.write_buffer(writer);


    client_conn->sendmsg(table_buffer, sizeof(message::standard::Header)+table_size);
    std::cerr << "Sent table containing " << table.size() << " entries:\n";
    for (auto it = table.cbegin(); it != table.cend(); ++it) {
        std::cerr << it->second.ip << ':' << it->second.port << '\n';
    }
    free(table_buffer);
}

static void handle_register(Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    uint16_t port_to_register = *(uint16_t*)(msg+sizeof(message::tracker::Header));
    std::string hash((char*)msg+sizeof(uint16_t)+sizeof(message::tracker::Header), (bufsize-sizeof(uint16_t)-sizeof(message::tracker::Header)));
    std::cout << "Got a Register request (hash=" << hash << ", port="<< port_to_register<<")" << std::endl;
    
    IPTable table;
    if (!session.get_table(hash, table)) { // Table does not exist yet, insert new table.
        // table.add_ip(client_conn->get_type(), client_conn->getAddress(), port_to_register);
        std::cerr << "Could not find table for hash " << hash << "\n";
        message::standard::send(client_conn, message::standard::ERROR);
    } else { // Table already exists. Add this peer to the list.
        if (session.add_peer(hash, {client_conn->get_type(), client_conn->getAddress(), port_to_register})) {
            std::cout << "Table with hash " << hash << " has "<< table.size() << " entries (added 1 new entry: " << client_conn->getAddress() << ':' << port_to_register << ")\n";
        } else {
            std::cout << "Table with hash " << hash << " has "<< table.size() << " entries (added 0 new entries)\n";
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
    std::cout << "Session initialized. Listening started on port " << port << std::endl;

    while (true) {
        auto client_conn = conn->acceptConnection();
        std::cout << "ClientConnection accepted with address " << client_conn->getAddress() << ':' << client_conn->getDestinationPort() << std::endl;

        message::standard::Header standard;
        
        if (!message::standard::recv(client_conn, standard)) {
            std::cout << "Unable to peek. System hangup?" << std::endl;
            continue;
        }

        if (standard.formatType != message::tracker::id) {
            std::cerr << "Received invalid message! Not a Tracker-message. Skipping..." << std::endl;
            continue;
        }
        uint8_t* ptr = (uint8_t*) malloc(standard.size);
        client_conn->recvmsg(ptr, standard.size);
        message::tracker::Header* header = (message::tracker::Header*) ptr;
        switch (header->tag) {
            case message::tracker::TEST:
                std::cout << "Got a test message" << std::endl;
                message::standard::send(client_conn, message::standard::OK);
                break;
            case message::tracker::MAKE_TORRENT: handle_make_torrent(session, client_conn, ptr, standard.size); break;
            case message::tracker::RECEIVE: handle_receive(session, client_conn, ptr, standard.size); break;
            case message::tracker::UPDATE:
                std::cout << "Got an update" << std::endl;
                //TODO: Implement update?
                message::standard::send(client_conn, message::standard::OK);
                break;
            case message::tracker::REGISTER: handle_register(session, client_conn, ptr, standard.size); break;
            default: std::cout << "Got unknown header tag: " << (uint16_t) header->tag << std::endl;break;
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