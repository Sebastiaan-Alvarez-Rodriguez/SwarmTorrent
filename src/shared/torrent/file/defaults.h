#ifndef TORRENT_DEFAULTS_H
#define TORRENT_DEFAULTS_H

namespace torrent::file::defaults {
    const static uint64_t fragment_size = 100*1024; // TODO: Use KB(100) function?
    const static size_t max_filehandles = 64; // Only allow up to this many open filehandles at a time for reading and writing (individually)
}

#endif