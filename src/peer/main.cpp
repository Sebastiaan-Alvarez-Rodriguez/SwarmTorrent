#include <iostream>
#include <tclap/CmdLine.h>

#include "shared/torrent/file/torrentFile.h"
#include "shared/connection/impl/TCP/in/TCPInConnection.h"
#include "shared/util/print.h"
#include "torrent/torrent.h"

// void make_torrent_file(std::string input_loc, std::string outputfile, TorrentFileOptions opts) {
//     TorrentFile.make_for(input_loc).with(opts).save(outputfile);
// }

// void make_torrent_file(std::string input_loc, std::string outputfile) {
//     TorrentFile::make_for(input_loc).save(outputfile);
// }


// Parse arguments for torrent::run and execute
bool run_torrent(int argc, char const ** argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Torrent", ' ', "0.1");
    TCLAP::ValueArg<std::string> torrentfileArg("f","file","The torrentfile to open",true,"","File", cmd);
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port for peer connections",true,1042,"PORT", cmd);
    cmd.parse(argc, argv);
    
    uint16_t port = portArg.getValue();
    std::string tf = torrentfileArg.getValue();
    if (port < 1000) {
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!"<<print::CLEAR<<std::endl;
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
    torrent     Torrent a file"
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

    return 0;
}