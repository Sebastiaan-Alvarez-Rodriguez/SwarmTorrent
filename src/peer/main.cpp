#include <iostream>

#include "shared/torrent/file/torrentFile.h"

// void make_torrent_file(std::string input_loc, std::string outputfile, TorrentFileOptions opts) {
//     TorrentFile.make_for(input_loc).with(opts).save(outputfile);
// }

void make_torrent_file(std::string input_loc, std::string outputfile) {
    TorrentFile::make_for(input_loc).save(outputfile);
}

int main(int argc, char const **argv) {
    std::cout << "I am a peer!" << std::endl;    
    return 0;
}