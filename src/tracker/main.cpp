#include <iostream>
#include <tclap/CmdLine.h>
#include <stdexcept>
#include <cstring>

#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/tracker/message.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/util/print.h"
#include "tracker/tracker/tracker.h"


bool run(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Tracker Run", ' ', "0.1");
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port for peer connections",true,1042,"PORT", cmd);
    cmd.parse(argc, argv);

    uint16_t port = portArg.getValue();
    if (port < 1000)
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!"<<print::CLEAR<<std::endl;

    auto conn = TCPHostConnection::Factory::from(NetType::IPv4).withPort(port).create();
    if (conn->get_state() != ClientConnection::READY) {
        std::cerr << print::RED << "[ERROR] Could not initialize connection" << print::CLEAR << std::endl;
        return false;
    }
    std::cout << "Listening started on port " << port << std::endl;

    bool running = true;
    while (running) {
        message::TrackerMessage m;
        auto client_conn = conn->acceptConnection();
        std::cout << "ClientConnection accepted with address "<< client_conn->getAddress() << ':' << client_conn->getPort() << std::endl;
        
        if (!client_conn->peekmsg((uint8_t*) &m.header, sizeof(message::TrackerMessage::Header))) {
            std::cout << "Unable to peek. System hangup?" << std::endl;
            return 1;
        }

        if (!m.valid()) {
            std::cerr << "Received invalid message! Not a Trackermessage. Skipping..." << std::endl;
            return 1;
        }
        uint8_t* ptr = (uint8_t*) malloc(m.header.size);
        std::memset(ptr, 0, m.header.size);
        
        client_conn->recvmsg(ptr, m.header.size);
        message::TrackerMessage::Header* header = (message::TrackerMessage::Header*) ptr;
        std::string hash((char*)ptr+sizeof(message::TrackerMessage::Header), (m.header.size-sizeof(message::TrackerMessage::Header)));
        std::cout << "The header size provided is '"<<m.header.size<<"', and trackerheader size is "<<sizeof(message::TrackerMessage::Header)<<", so hash string length is "<<(m.header.size-sizeof(message::TrackerMessage::Header))<<'\n';
        std::cout << "Here is the hash: '"<<hash<<"'\n";
        switch (header->tag) {
            case message::TrackerMessage::Tag::SUBSCRIBE: std::cout << "Got a subscribe" << std::endl; break;
            case message::TrackerMessage::Tag::UNSUBSCRIBE: std::cout << "Got an unsubscribe" << std::endl; break;
            case message::TrackerMessage::Tag::RECEIVE: std::cout << "Got a receive" << std::endl; break;
            case message::TrackerMessage::Tag::UPDATE: std::cout << "Got an update" << std::endl;break;
            default: std::cout << "Got unknown header tag: " << (uint16_t) header->tag << std::endl;break;
        }
    }
    return 0;
}

int main(int argc, char const **argv) {
    run(argc, argv);
    return 0;
}