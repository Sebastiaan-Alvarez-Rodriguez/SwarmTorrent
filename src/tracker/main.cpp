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

void handle_receive(const Session& session, std::unique_ptr<ClientConnection>& client_conn, const std::string& hash) {
    std::cout << "Got a receive." << std::endl;
    IPTable table;
    if (!session.get_table(hash, table)) { //No table for hash found, return error
        message::standard::send(client_conn, message::standard::type::ERROR);
        return;
    }
    //TODO @Mariska: send the found peertable to the client here, instead of sending OK.
    message::standard::send(client_conn, message::standard::type::OK);
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

    bool running = true;
    while (running) {
        message::standard::Header standard;
        auto client_conn = conn->acceptConnection();
        std::cout << "ClientConnection accepted with address "<< client_conn->getAddress() << ':' << client_conn->getPort() << std::endl;
        
        if (!message::standard::from(client_conn, standard)) {
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
        std::string hash((char*)ptr+sizeof(message::tracker::Header), (standard.size-sizeof(message::tracker::Header)));
        std::cout << "The header size provided is '"<<standard.size<<"', and trackerheader size is "<<sizeof(message::tracker::Header)<<", so hash string length is "<<(standard.size-sizeof(message::tracker::Header))<<'\n';
        std::cout << "Here is the hash: '"<<hash<<"'\n";
        switch (header->tag) {
            case message::tracker::Tag::SUBSCRIBE:
                std::cout << "Got a subscribe" << std::endl;
                message::standard::send(client_conn, message::standard::type::OK);
                break;
            case message::tracker::Tag::UNSUBSCRIBE:
                std::cout << "Got an unsubscribe" << std::endl;
                message::standard::send(client_conn, message::standard::type::OK);
                break;
            case message::tracker::Tag::RECEIVE:
                handle_receive(session, client_conn, hash);
                break;
            case message::tracker::Tag::UPDATE:
                std::cout << "Got an update" << std::endl;
                message::standard::send(client_conn, message::standard::type::OK);
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