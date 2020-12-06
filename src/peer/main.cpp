#include <iostream>
#include <tclap/CmdLine.h>

#include "connections/tracker/connections.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/tracker/message.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"
#include "torrent/torrent.h"

void do_test(int argc, char const ** argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Test", ' ', "0.1");
    TCLAP::ValueArg<std::string> addrArg("a","address","Address of host",false,"127.0.0.1","ADDR", cmd);
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port of host",true,1042,"PORT", cmd);
    TCLAP::ValueArg<uint16_t> sendArg("s","sendarg","Argument to send",false, (uint16_t) message::tracker::Tag::TEST, "ARG", cmd);
    
    cmd.parse(argc, argv);

    std::string addr = addrArg.getValue();
    uint16_t port = portArg.getValue();
    uint16_t tag = sendArg.getValue();

     if (port < 1000)
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!" << print::CLEAR << std::endl;
    
    auto tracker_conn = TCPClientConnection::Factory::from(NetType::IPv4).withAddress(addr).withPort(port).create();
    std::string torrent_hash = "some cool hash";
    if (!tracker_conn->doConnect()) {
        std::cerr << print::RED << "[ERROR] Could not connect to remote!" << print::CLEAR << std::endl;
        return;
    }

    IPTable peertable;
    message::standard::Header h;
    switch ((uint8_t) tag) {
        case 0:
            connections::tracker::test(tracker_conn, torrent_hash); break;
        case 1:
            std::cerr << "Cannot send a MAKE_TORRENT request. use 'make' instead of 'test'\n"; 
            return;
        case 3:
            // return torrent::make(in, out, trackers);
            // TODO @Mariska: No idea what you are doing here. Please fix this stuff yourself.
            // peertable.add_ip(Address(ConnectionType(TransportType::Type::TCP, NetType::Type::IPv4), addr, port));
            // connections::tracker::make_torrent(tracker_conn, torrent_hash, peertable); 
            // if (message::standard::from(tracker_conn, h) && h.formatType == message::standard::type::OK) {
            //     std::cout << "Received OK!\n";
            //     tracker_conn->recvmsg((uint8_t*)&h, sizeof(h));
            // } else {
            //     std::cout << "Received Nothing, bye!\n";
            //     return;
            // }
            // std::cout << "Sending Receive Request" << std::endl;
            // if (!connections::tracker::receive(tracker_conn, torrent_hash, peertable)) {
            //     std::cerr << "could not receive" << std::endl;
            //     return;
            // }
            break;
        default:
            std::cout << "Did not send anything" << std::endl;    
    }
    if (message::standard::from(tracker_conn, h) && h.formatType == message::standard::type::OK) {
        std::cout << "Received OK, bye!\n";
    } else {
        std::cout << "Received Nothing, bye!\n";
    }
}

// Parse arguments for torrent::run and execute
bool run_torrent(int argc, char const ** argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Torrent", ' ', "0.1");
    TCLAP::ValueArg<std::string> torrentfileArg("f","file","The torrentfile to open",true,"","File", cmd);
    cmd.parse(argc, argv);
    
    std::string tf = torrentfileArg.getValue();
    return torrent::run(tf);
}

// Parse arguments for torrent::make and execute
bool make_torrent(int argc, char const ** argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Make_Torrent", ' ', "0.2");
    TCLAP::ValueArg<std::string> inArg("i","input-file","file to make a torrentfile for",true,"","FILE", cmd);
    TCLAP::ValueArg<std::string> outArg("o","output-file","Output location for torrentfile",true,"","FILE", cmd);
    TCLAP::MultiArg<std::string> trackerArgs("t", "trackers", "Tracker IPs format: [TCP]:[4/6]:PORT:IP", true, "IP strings", cmd);
    
    cmd.parse(argc, argv);
    
    std::string in = inArg.getValue(), out = outArg.getValue();
    std::vector<std::string> trackers = trackerArgs.getValue();
    return torrent::make(in, out, trackers);
}

// TCLAP manual: http://tclap.sourceforge.net/manual.html
int main(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Main", ' ', "0.1");

    if (argc < 2) {
        std::cerr << R"(
Help

Give a subcommand to the peer. Options:
    make        Construct a torrentfile
    torrent     Torrent a file
    test        Call test function
)";
        return 1;
    }
    
    std::string subcommand = std::string(argv[1]);
    argc -= 1;
    auto argv_mvd = argv+1;
    if (subcommand == "make")
        return make_torrent(argc, argv_mvd);
    else if (subcommand == "torrent")
        return run_torrent(argc, argv_mvd);
    else if (subcommand == "test")
        do_test(argc, argv_mvd);
    return 0;
}