#ifndef PEER_TORRENT_H
#define PEER_TORRENT_H

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
namespace torrent {

    // Get to work on a torrent, and let peers connect on given port
    bool run(uint16_t port);

    // Make a torrentfile from path in, write created torrentfile to out
    bool make(std::string in, std::string out, std::vector<std::string> trackers);
}
#endif