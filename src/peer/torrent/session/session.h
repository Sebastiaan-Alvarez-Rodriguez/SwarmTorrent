#ifndef PEER_TORRENT_SESSION_H
#define PEER_TORRENT_SESSION_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "shared/connection/connection.h"
#include "shared/torrent/file/defaults.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/torrent/hashTable/hashTable.h"
#include "shared/torrent/metadata/metaData.h"

namespace torrent {
    class Registry {
    public:
        Registry() = default;
        ~Registry() = default;
        
        inline void request_add(size_t fragment_idx, const Address& address) {
            //TODO: Nothing yet
        }

        inline void request_complete(size_t fragment_idx) {
            //TODO: Nothing yet
        }

        inline std::vector<size_t> get_stale_requests() {
            auto vec = std::vector<size_t>();
            //TODO: Nothing yet
            return vec;
        }

        inline size_t size() const {
            return 10;
        }
    };

    class Session {
    protected:
        const HashTable htable;
        const TorrentMetadata metadata;

        std::string workpath; // Base path to prepend to suggested names for files
        std::unordered_map<std::string, FragmentHandler> fileHandles; // Provides a 'cache' to fileHandles

        std::shared_ptr<HostConnection> recv_conn;

        Registry registry;
        IPTable ptable;

        size_t num_fragments;
        size_t num_fragments_completed = 0;
        std::vector<bool> fragments_completed;

    public:
        /**
         * Constructs a session.
         *
         * @param tf The torrentfile to use for torrenting
         * @param recv_conn The connection which will receive all requests.
         *
         * '''Note:''' Ownership of `recv_conn` is passed to this session upon construction.
         *             Connection is closed when the session is deconstructed.
         */
        explicit Session(const TorrentFile& tf, std::unique_ptr<HostConnection> recv_conn, std::string workpath) : htable(tf.getHashTable()), metadata(tf.getMetadata()), workpath(workpath), recv_conn(std::move(recv_conn)), num_fragments(metadata.get_num_fragments()), fragments_completed(num_fragments, false) {}

        inline void mark(size_t index) {
            if (!fragments_completed[index]) {
                fragments_completed[index] = true;
                ++num_fragments_completed;
            }
        }

        inline bool download_completed() const {
            return num_fragments == num_fragments_completed;
        }

        inline const auto& get_hashtable() const { return htable; }

        inline const auto& get_metadata() const { return metadata; }

        inline FragmentHandler& getHandler(const std::string& suggested_name) {

            // TODO @Mariska, maximum priority: Finish caching system
            auto it = fileHandles.find(suggested_name);
            if (it != fileHandles.end())
                return it->second;
            const std::string fileloc = workpath + metadata.name;
            if (fileHandles.size() < torrent::file::defaults::max_filehandles) {
                auto test = std::make_pair<std::string, FragmentHandler>(std::string(suggested_name), FragmentHandler(metadata, fileloc));
                return fileHandles.insert(test).first->second;
            } else { //TODO: Cannot have too many open filehandles. Close a fragmenthandler
                throw std::runtime_error("Did not implement fragmentHandler recycling!");
            }
        }

        inline const auto& get_conn() const { return recv_conn; }

        inline const auto& get_registry() const { return registry; }

        inline const auto& get_peertable() const { return ptable; }

        // inline auto& peertable() { return ptable; }
        inline bool add_peer(const Address& a) { return ptable.add_ip(a); }
        inline bool add_peer(ConnectionType type, const std::string& ip, uint16_t port) { return ptable.add_ip(type, ip, port); }  

        inline bool remove_peer(const std::string& ip, uint16_t port) {
            Address a;
            if (!ptable.get_addr(ip, a))
                return false;
            if (a.ip == ip && a.port == port) {// Only remove peer if both ip and port match
                ptable.remove_ip(ip);
                return true;
            }
            return false;
        }

        inline size_t peers_amount() { return ptable.size(); }
    };
}
#endif