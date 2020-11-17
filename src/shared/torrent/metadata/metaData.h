#ifndef TORRENT_METADATA_H
#define TORRENT_METADATA_H

#include <cstdint>
#include <iostream>
#include <string>

#include "shared/torrent/file/streamable/streamable.h"
#include "shared/torrent/file/defaults.h"

struct TorrentMetadata : public Streamable {
    std::string name; // Suggested name to save the file
    uint64_t size; //Total size of the file we stream, in bytes
    uint64_t fragment_size = torrent::file::defaults::fragment_size; // Size of one fragment, in bytes

    void read_stream(std::istream& stream) override;

    void write_stream(std::ostream& stream) const override;

    static TorrentMetadata from(std::istream stream) {
        TorrentMetadata m;
        m.read_stream(stream);
        return m;
    }
};
#endif