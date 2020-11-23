#include <iostream>
#include <tclap/CmdLine.h>
#include <stdexcept>

#include "shared/connection/impl/TCP/in/TCPInConnection.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/util/print.h"


bool setup(int argc, char const ** argv, IPTable& pt, std::unique_ptr<Connection>& tracker_conn) {
    TCLAP::CmdLine cmd("SwarmTorrent Tracker Main", ' ', "0.1");
    TCLAP::ValueArg<std::string> seederArg("i", "initial-seeder", "IP of initial seeder, format: [TCP]:[4/6]:PORT:IP", true, "", "IP Strings", cmd);
    TCLAP::ValueArg<unsigned> portArg("p", "port", "Specify on which port to listen", true, 0, "unsigned", cmd);

    cmd.parse(argc, argv);

    std::string initial_seeder = seederArg.getValue();
    try {
        std::vector<std::string> peers = {initial_seeder};
        pt = IPTable::from(peers);
    } catch (const std::exception& e) {
        std::cerr << print::RED << "[ERROR] Could not setup PeerTable: " << e.what() << std::endl;
        return false;
    }

    unsigned tracker_port = portArg.getValue();
    if (tracker_port < 1000) 
        std::cerr << print::YELLOW << "[WARNING] Port numbers lower than 1000 may be reserved by the OS!"<< print::CLEAR << std::endl;

    tracker_conn = TCPInConnection::Factory::from(NetType::IPv4).withPort(tracker_port).create();
    if (tracker_conn->get_state() != Connection::READY) {
        std::cerr << print::RED << "[ERROR] Could not initialize connection" << std::endl;
        return false;
    }

    return true;
}

//TODO: change to multiple torrentfiles
int main(int argc, char const **argv) {
    IPTable pt;
    std::unique_ptr<Connection> tracker_conn;
    if (!setup(argc, argv, pt, tracker_conn))
        return 1;
    
    return 0;
}