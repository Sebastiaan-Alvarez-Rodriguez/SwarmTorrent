#include <fstream>
#include <iostream>

#include "fragmentHandler.h"
#include "../hash/hasher.h"

bool FragmentHandler::write_fragment(unsigned index, const uint8_t* data, unsigned data_size) {
    if (fileinfo.has_fragment(index))
        return false;

    const TorrentFile& torrentfile = fileinfo.get_torrentfile();
    std::string hash;
    hash_sha256(hash, data, data_size);
    if (!torrentfile.check_hash(index, hash))
        return false;

    const unsigned frag_size = torrentfile.get_fragment_size();

    std::ofstream f;
    f.open(fileinfo.get_path(), std::ios::binary);
    f.seekp(index * frag_size, std::ios::beg);
    f.write((char*)data, data_size);
    f.close();

    fileinfo.set_received(index);
    may_seed = torrentfile.can_seed(fileinfo.get_nr_fragments());
    return true;
} 

bool FragmentHandler::read_fragment(unsigned index, uint8_t* data, unsigned& data_size) {
    if (!fileinfo.has_fragment(index))
        return false;

    const TorrentFile& torrentfile = fileinfo.get_torrentfile();
    const unsigned frag_size = torrentfile.get_fragment_size();
    data_size = (index != torrentfile.get_nr_fragments()-1) ?  frag_size : torrentfile.get_file_size() % frag_size;
    data = new uint8_t[data_size];

    std::ifstream f;
    f.open(fileinfo.get_path(), std::ios::binary);
    f.seekg(index * frag_size, std::ios::beg);
    f.read((char*)data, data_size);
    f.close();
    return true;
}