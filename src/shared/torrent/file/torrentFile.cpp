#include <stdexcept>
#include <iostream>
#include <fstream>

#include "shared/torrent/hashTable/hashTable.h"
#include "shared/torrent/metadata/metadata.h"
#include "shared/torrent/trackerTable/trackerTable.h"
#include "shared/util/fs/fs.h"

#include "torrentFile.h"


TorrentFile TorrentFile::from(std::string path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
        throw std::runtime_error("There was an error opening this file");

    auto tb = TrackerTable();
    auto tm = TorrentMetadata();
    auto ht = HashTable();

    tb.read_stream(stream);
    tm.read_stream(stream);
    ht.read_stream(stream);
    stream.close();
    return TorrentFile(tb, tm, ht);
}

TorrentFile TorrentFile::make_for(std::string path) {
    if (!fs::is_file(path))
        throw std::runtime_error("Can only open files for now!");
    auto tb = TrackerTable::empty(); //TODO Mariska: Init trackertable when a peer calls this?
    TorrentMetadata tm;
    tm.name = fs::basename(path);
    tm.size = fs::file_size(path);
    auto ht = HashTable::make_for(path);
    return TorrentFile(tb, tm, ht);
}


// Write the contents of the TorrentFile 
void TorrentFile::save(std::string path) {
    std::ofstream stream; 
    stream.open(path, std::ios::trunc | std::ios::binary);
    trackertable.write_stream(stream);
    metadata.write_stream(stream);
    hashtable.write_stream(stream);
    stream.close();
}
