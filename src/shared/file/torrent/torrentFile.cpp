#include <stdexcept>
#include <iostream>
#include <fstream>

#include "torrentFile.h"

TorrentFile::TorrentFile(std::string path) {
    std::ifstream swarmtorrent; 
    swarmtorrent.open(path, std::ios::binary);
    if (!swarmtorrent.is_open())
        throw std::runtime_error("There was an error opening this file");

    trackertable.read_swarm(swarmtorrent);
    unsigned name_length;
    swarmtorrent.read((char*)(&name_length), sizeof(name_length));
    swarmtorrent.read((char*)name.data(), name_length);
    swarmtorrent.read((char*)(&length), sizeof(length));
    swarmtorrent.read((char*)(&fragment_length), sizeof(fragment_length));
    swarmtorrent.read((char*)(&seed_threshold), sizeof(seed_threshold));
    hashtable.read_swarm(swarmtorrent);
    swarmtorrent.close();
    this->path = path;

}

//Write the contents of the TorrentFile 
void TorrentFile::write(std::string path) {
    std::ofstream swarmtorrent; 
    swarmtorrent.open(path, std::ios::trunc | std::ios::binary);
    trackertable.write_swarm(swarmtorrent);
    unsigned name_length = name.length();
    swarmtorrent.write((char*)(&name_length), sizeof(name_length));
    swarmtorrent.write((char*)name.data(), name_length);
    swarmtorrent.write((char*)(&length), sizeof(length));
    swarmtorrent.write((char*)(&fragment_length), sizeof(fragment_length));
    swarmtorrent.write((char*)(&seed_threshold), sizeof(seed_threshold));
    hashtable.write_swarm(swarmtorrent);
    swarmtorrent.close();
    this->path = path;
}
