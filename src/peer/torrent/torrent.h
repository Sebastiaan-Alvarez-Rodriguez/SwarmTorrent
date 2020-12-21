#ifndef PEER_TORRENT_H
#define PEER_TORRENT_H

#include <iostream>
#include <tclap/CmdLine.h>

#include "shared/torrent/file/torrentFile.h"
#include "shared/util/print.h"

namespace torrent {
    // Make a torrentfile from path in, write created torrentfile to out
    bool make(const std::string& in, const std::string& out, std::vector<std::string>& trackers);

    // Torrent a torrentfile provided by torrentfile, with given workpath
    bool run(const std::string& torrentfile, const std::string& workpath, uint16_t port, bool initial_seeder, const std::string& logfile);
}
#endif