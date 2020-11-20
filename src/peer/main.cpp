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


// Parse arguments for torrent::run and execute
bool run_torrent() {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Torrent", ' ', "0.1");
    TCLAP::ValueArg<std::string> torrentfileArg("f","file","The torrentfile to open",true,1042,"PORT", cmd);
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port for peer connections",true,1042,"PORT", cmd);
    cmd.parse(argc, argv);
    
    std::string tf = torrentfileArg.getValue();
    uint16_t port = portArg.getValue();
    return torrent::run(port);
}

// Parse arguments for torrent::make and execute
bool make_torrent() {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Make_Torrent", ' ', "0.1");
    TCLAP::ValueArg<std::string> inArg("i","input-file","file to make a torrentfile for",true,"","FILE", cmd);
    TCLAP::ValueArg<std::string> outArg("o","output-file","Output location for torrentfile",true,"","FILE", cmd);
    
    cmd.parse(argc, argv);
    
    std::string in = inArg.getValue(), out = outArg.getValue();
    return torrent::make(in, out);
}

// Determine which subcommand to call
bool subcommand(std::string subcommand) {
    if (subcommand == "torrent")
        return torrent();
    else if (subcommand == "make")
        return make_torrent();
}

// TCLAP manual: http://tclap.sourceforge.net/manual.html
int main(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Peer Main", ' ', "0.1");

    if (port < 1000) {
        std::cerr << print::YELLOW << "Port numbers lower than 1000 may be reserved by the OS!"<<print::CLEAR<<std::endl;
    }

    std::vector<std::string> tmp;
    tmp.push_back("torrent");
    tmp.push_back("make");
    ValuesConstraint<string> allowed(tmp);
    
    ValueArg<std::string> commArg("c","command","Command to execute",true,"torrent",&allowed, cmd);
    cmd.parse(argc, argv);

    return subcommand(commArg.getValue()) ? 0 : 1;
}