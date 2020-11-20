#include <iostream>
#include <tclap/CmdLine.h>

#include "shared/torrent/file/torrentFile.h"
#include "shared/connection/impl/TCP/in/TCPInConnection.h"
#include "shared/connection/impl/TCP/out/TCPOutConnection.h"
#include "shared/util/print.h"

// void make_torrent_file(std::string input_loc, std::string outputfile, TorrentFileOptions opts) {
//     TorrentFile.make_for(input_loc).with(opts).save(outputfile);
// }

// void make_torrent_file(std::string input_loc, std::string outputfile) {
//     TorrentFile::make_for(input_loc).save(outputfile);
// }

bool start(std::string address_tracker, uint16_t port_tracker, uint16_t port_peer) {
    auto conn_peer = TCPInConnection::Factory::from(NetType::IPv4).withPort(port_peer).create();
    auto conn_tracker = TCPOutConnection::Factory::from(NetType::IPv4).withAddress(address_tracker).withPort(port_tracker).create();

    std::cout << "Peer: ";
    if (conn_peer->get_state() == Connection::READY)
        std::cout << "Successfully connected";
    else
        std::cout << "Failure while connecting";
    std::cout << " with Connection object: " << *conn_peer << std::endl;


    std::cout << "Tracker: ";
    if (conn_tracker->get_state() == Connection::READY)
        std::cout << "Successfully connected";
    else
        std::cout << "Failure while connecting";
    std::cout << " with Connection object: " << *conn_tracker << std::endl;

    return conn_peer->get_state() == Connection::READY && conn_tracker->get_state() == Connection::READY;
}

// TCLAP manual: http://tclap.sourceforge.net/manual.html
int main(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent peer", ' ', "0.1");

    TCLAP::ValueArg<std::string> addressArg("","address","Address for connection",true,"","NAME", cmd);
    TCLAP::ValueArg<uint16_t> portpeerArg("","port-peer","Port for peer connections",true,42,"PORT", cmd);
    TCLAP::ValueArg<uint16_t> porttrackerArg("","port-tracker","Port for tracker connections",true,43,"PORT", cmd);
    
    cmd.parse(argc, argv);
    std::string address = addressArg.getValue();

    uint16_t port_peer = portpeerArg.getValue();
    uint16_t port_tracker = porttrackerArg.getValue();
    
    if (port_peer == port_tracker) {
        std::cerr << print::RED << "[ERROR] Port for peer and tracker connections must differ"<<print::CLEAR<<" (now both" << port_peer << ")" << std::endl;
        return 1;
    }
    if (port_peer < 1000 || port_tracker < 1000) {

    }

    start(address, port_tracker, port_peer);
    return 0;
}