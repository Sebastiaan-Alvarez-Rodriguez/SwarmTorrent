#include <iostream>
#include <tclap/CmdLine.h>

#include "connections/tracker/tracker.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/tracker/message.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"
#include "torrent/torrent.h"

// void make_torrent_file(std::string input_loc, std::string outputfile, TorrentFileOptions opts) {
//     TorrentFile.make_for(input_loc).with(opts).save(outputfile);
// }

// void make_torrent_file(std::string input_loc, std::string outputfile) {
//     TorrentFile::make_for(input_loc).save(outputfile);
// }

void do_test(int argc, char const ** argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Test", ' ', "0.1");
    TCLAP::ValueArg<std::string> addrArg("a","address","Address of host",false,"127.0.0.1","ADDR", cmd);
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port of host",true,1042,"PORT", cmd);
    TCLAP::ValueArg<uint16_t> sendArg("s","sendarg","Argument to send",false, (uint16_t) message::tracker::Tag::SUBSCRIBE, "ARG", cmd);
    
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

    switch ((uint8_t) tag) {
        case 0:
            connections::tracker::subscribe(tracker_conn, torrent_hash); break;
        case 1:
            connections::tracker::unsubscribe(tracker_conn, torrent_hash); break;
        default:
            std::cout << "Did not send anything" << std::endl;    
    }
    message::standard::Header h;
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
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port for peer connections",true,1042,"PORT", cmd);
    cmd.parse(argc, argv);
    
    uint16_t port = portArg.getValue();
    std::string tf = torrentfileArg.getValue();
    if (port < 1000) {
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!" << print::CLEAR << std::endl;
    }
    return torrent::run(port);
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