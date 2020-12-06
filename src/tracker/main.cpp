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


void handle_receive(const Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    std::string hash((char*)msg+sizeof(message::tracker::Header), (bufsize-sizeof(message::tracker::Header)));
    std::cout << "Got a Receive request (hash=" << hash << ")" << std::endl;

    IPTable table;
    if (!session.get_table(hash, table)) { //No table for hash found, return error
        message::standard::send(client_conn, message::standard::ERROR);
        std::cerr << "Error" << std::endl;
        return;
    }
    size_t buf_length = table.size() * Address::size() + sizeof(message::standard::Header);
    uint8_t* const table_buffer = (uint8_t*)malloc(buf_length);
    uint8_t* writer = table_buffer;
    *((message::standard::Header*) writer) = message::standard::from(table.size()*Address::size(), message::standard::OK);
    writer += sizeof(message::standard::Header);
    for (auto it = table.iterator_begin(); it != table.iterator_end(); ++it) 
        writer = (*it).second.write_buffer(writer);
    client_conn->sendmsg(table_buffer, buf_length);
    free(table_buffer);
}

void handle_make_torrent(Session& session, std::unique_ptr<ClientConnection>& client_conn, const uint8_t* const msg, size_t bufsize) {
    std::string hash((char*)msg+sizeof(message::tracker::Header), (bufsize-sizeof(message::tracker::Header)));
    std::cout << "Got a Make Torrent request (hash=" << hash << ")" << std::endl;

    IPTable table;
    if (!session.get_table(hash, table)) { // Table does not exist yet, insert new table.
        auto addr = client_conn->getAddress();
        auto port = client_conn->getPort();
        table.add_ip({TransportType::TCP, NetType::IPv4}, addr, port);
        session.add_table(hash, table);
    } else { // Table already exists. Add this peer to the list.
        auto addr = client_conn->getAddress();
        auto port = client_conn->getPort();
        session.add_peer(hash, {{TransportType::TCP, NetType::IPv4}, addr, port});
    }
    message::standard::send(client_conn, message::standard::OK);
}

bool run(uint16_t port) {
    auto conn = TCPHostConnection::Factory::from(NetType::IPv4).withPort(port).create();
    if (conn->get_state() != ClientConnection::READY) {
        std::cerr << print::RED << "[ERROR] Could not initialize connection" << print::CLEAR << std::endl;
        return false;
    }

    Session session;
    std::cout << "Session initialized" << std::endl;
    std::cout << "Listening started on port " << port << std::endl;

    // bool running = true;

    auto client_conn = conn->acceptConnection();
    std::cout << "ClientConnection accepted with address "<< client_conn->getAddress() << ':' << client_conn->getPort() << std::endl;
    while (true) {
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
            case message::tracker::Tag::TEST:
                std::cout << "Got a test message" << std::endl;
                message::standard::send(client_conn, message::standard::OK);
                break;
            case message::tracker::Tag::MAKE_TORRENT: handle_make_torrent(session, client_conn, ptr, standard.size); break;
            case message::tracker::Tag::RECEIVE: {
                std::cout << "receive" << std::endl;
                handle_receive(session, client_conn, ptr, standard.size); break;
            }
            case message::tracker::Tag::UPDATE:
                std::cout << "Got an update" << std::endl;
                message::standard::send(client_conn, message::standard::OK);
                break;
            default: std::cout << "Got unknown header tag: " << (uint16_t) header->tag << std::endl;break;
        }
    }
    return 0;
}

int main(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Tracker Run", ' ', "0.1");
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port for peer connections",true,1042,"PORT", cmd);
    cmd.parse(argc, argv);

    uint16_t port = portArg.getValue();
    if (port < 1000)
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!"<<print::CLEAR<<std::endl;

    run(port);
    return 0;
}