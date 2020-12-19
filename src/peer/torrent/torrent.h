#ifndef PEER_TORRENT_H
#define PEER_TORRENT_H

#include <iostream>
#include <tclap/CmdLine.h>

#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"

// void make_torrent_file(std::string input_loc, std::string outputfile, TorrentFileOptions opts) {
//     TorrentFile.make_for(input_loc).with(opts).save(outputfile);
// }

// void make_torrent_file(std::string input_loc, std::string outputfile) {
//     TorrentFile::make_for(input_loc).save(outputfile);
// }
namespace torrent {
    // Make a torrentfile from path in, write created torrentfile to out
    bool make(const std::string& in, const std::string& out, std::vector<std::string>& trackers);

    // Torrent a torrentfile provided by torrentfile, with given workpath
    bool run(const std::string& torrentfile, const std::string& workpath, uint16_t port, bool force_register, const std::string& logfile);
}
#endif