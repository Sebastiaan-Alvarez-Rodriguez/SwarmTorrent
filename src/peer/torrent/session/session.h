#ifndef PEER_TORRENT_SESSION_H
#define PEER_TORRENT_SESSION_H

#include <chrono>
#include <cstdint>
#include <memory>

#include "shared/connection/connection.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/torrent/hashTable/hashTable.h"
#include "shared/torrent/metadata/metaData.h"
#include "registry/registry.h"

namespace torrent {
    class Session {
    protected:
        const HashTable htable;
        const TorrentMetadata metadata;

        FragmentHandler fragmentHandler;

        std::shared_ptr<HostConnection> recv_conn;
        Address own_address;

        Registry registry;
        IPTable ptable;

        const size_t num_fragments;
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
        explicit Session(const TorrentFile& tf, std::unique_ptr<HostConnection> recv_conn, std::string workpath) : htable(tf.getHashTable()), metadata(tf.getMetadata()), fragmentHandler(metadata, workpath + metadata.name), recv_conn(std::move(recv_conn)), num_fragments(metadata.get_num_fragments()), fragments_completed(num_fragments, false) {}

        inline void mark(size_t fragment_nr) {
            if (!fragments_completed[fragment_nr]) {
                fragments_completed[fragment_nr] = true;
                ++num_fragments_completed;
                registry.remove(fragment_nr); // Remove requests for collected fragment
            }
        }

        inline bool fragment_completed(size_t fragment_nr) {
            return fragments_completed[fragment_nr];
        }
        inline size_t get_num_fragments() {
            return num_fragments;
        }

        inline bool download_completed() const {
            return num_fragments == num_fragments_completed;
        }

        inline const auto& get_hashtable() const { return htable; }

        inline const auto& get_metadata() const { return metadata; }

        inline auto& get_handler() {
            return fragmentHandler;
        }
        inline const auto& get_conn() const { return recv_conn; }

        inline void set_address(Address& address) { own_address = address; }

        inline const auto& get_address() const { return own_address; }

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
        inline bool has_registered_peer(const std::string& ip) {
            return ptable.contains(ip);
        }

        inline size_t peers_amount() { return ptable.size(); }
    };
}
#endif