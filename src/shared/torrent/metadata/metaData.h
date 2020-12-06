#ifndef TORRENT_METADATA_H
#define TORRENT_METADATA_H

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

#include "shared/torrent/file/streamable/streamable.h"
#include "shared/torrent/file/defaults.h"

struct TorrentMetadata : public Streamable {
    TorrentMetadata() = default;
    TorrentMetadata(std::string name, uint64_t size, uint64_t fragment_size, std::string hash) : name(std::move(name)), content_hash(hash), size(size), fragment_size(fragment_size)  {}
    
    std::string name; // Suggested name to save the file
    std::string content_hash; // Hash of the content of the complete file
    uint64_t size; //Total size of the file we stream, in bytes
    uint64_t fragment_size = torrent::file::defaults::fragment_size; // Size of one fragment, in bytes

    // Returns the number of fragments the file has
    inline uint64_t get_num_fragments() const { return ((size-1) / fragment_size)+1; };

    // Returns the size of the fragments in bytes
    inline uint64_t get_fragment_size() const { return fragment_size; };
  
    void read_stream(std::istream& stream) override;

    void write_stream(std::ostream& stream) const override;

    static TorrentMetadata from(std::istream stream) {
        TorrentMetadata m;
        m.read_stream(stream);
        return m;
    }
};
#endif