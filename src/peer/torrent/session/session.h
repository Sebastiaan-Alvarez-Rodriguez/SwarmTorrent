#ifndef PEER_TORRENT_SESSION_H
#define PEER_TORRENT_SESSION_H

#include <vector>
#include <cstdint>

#include "shared/torrent/file/torrentFile.h"
#include "shared/torrent/hashTable/hashTable.h"
#include "shared/torrent/metadata/metaData.h"

namespace torrent {
    class Session {
    protected:
        const HashTable htable;
        const TorrentMetadata metadata;

        size_t num_fragments;
        size_t num_fragments_completed = 0;
        std::vector<bool> fragments_completed;
        
        // std::vector<std::unique_ptr<Client

    public:
        explicit Session(const TorrentFile& tf) : htable(tf.getHashTable()), metadata(tf.getMetadata()), num_fragments(metadata.get_num_fragments()), fragments_completed(num_fragments, false) {}

        inline const HashTable& table() const { return htable; }

        inline void mark(size_t index) {
            if (!fragments_completed[index]) {
                fragments_completed[index] = true;
                ++num_fragments_completed;
            }
        }

        inline bool download_completed() const {
            return num_fragments == num_fragments_completed;
        }
    };
}
#endif