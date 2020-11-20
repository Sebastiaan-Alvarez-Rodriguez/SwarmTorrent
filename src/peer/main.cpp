#include <iostream>
#include <tclap/CmdLine.h>

#include "shared/torrent/file/torrentFile.h"
#include "shared/connection/impl/TCP/factory.h"

// void make_torrent_file(std::string input_loc, std::string outputfile, TorrentFileOptions opts) {
//     TorrentFile.make_for(input_loc).with(opts).save(outputfile);
// }

// void make_torrent_file(std::string input_loc, std::string outputfile) {
//     TorrentFile::make_for(input_loc).save(outputfile);
// }

// TCLAP manual: http://tclap.sourceforge.net/manual.html
int main(int argc, char const **argv) {
    TCLAP::CmdLine cmd("SwarmTorrent peer", ' ', "0.1");

    TCLAP::ValueArg<std::string> addressArg("a","address","Address for connection",true,"","NAME", cmd);
    TCLAP::ValueArg<uint16_t> portArg("p","port","Port for connection",true,42,"PORT", cmd);
    
    cmd.parse(argc, argv);
    std::string address = addressArg.getValue();
    uint16_t port = portArg.getValue();
    auto f = TCPFactory::from(NetType::IPv4).withAddress(address).withPort(port).create();
    std::cout << "Peer: Successfully build Connection object: " << *f << std::endl;
    return 0;
}