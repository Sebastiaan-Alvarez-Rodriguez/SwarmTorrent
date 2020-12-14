#ifndef PEER_TORRENT_SESSION_H
#define PEER_TORRENT_SESSION_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <random>

#include "shared/connection/connection.h"
#include "shared/torrent/file/torrentFile.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/torrent/hashTable/hashTable.h"
#include "shared/torrent/metadata/metaData.h"
#include "shared/util/random/random.h"
#include "shared/util/random/randomGenerator.h"
#include "registry/peer/registry.h"
#include "registry/request/registry.h"

namespace torrent {
    class Session {
    protected:
        const HashTable htable;
        const TorrentMetadata metadata;

        FragmentHandler fragmentHandler;

        std::shared_ptr<HostConnection> recv_conn;
        Address own_address;

        peer::Registry peer_registry;
        request::Registry request_registry;
        IPTable ptable; // table containing peers we might join. For joined peers, see [[peer_registry]]

        const size_t num_fragments;
        size_t num_fragments_completed = 0;
        std::vector<bool> fragments_completed;

    public:
        // Simple random number generator to use during this session.
        // Initialized such that different peers generate different numbers
        rnd::RandomGenerator<size_t> rand;
        
        /**
         * Constructs a session.
         *
         * @param tf The torrentfile to use for torrenting
         * @param recv_conn The connection which will receive all requests.
         *
         * '''Note:''' Ownership of `recv_conn` is passed to this session upon construction.
         *             Connection is closed when the session is deconstructed.
         */
        explicit Session(const TorrentFile& tf, std::unique_ptr<HostConnection> recv_conn, std::string workpath) : htable(tf.getHashTable()), metadata(tf.getMetadata()), fragmentHandler(metadata, workpath + metadata.name), recv_conn(std::move(recv_conn)), num_fragments(metadata.get_num_fragments()), fragments_completed(num_fragments, false), rand(std::random_device()) {}

        inline void mark_fragment(size_t fragment_nr) {
            if (!fragments_completed[fragment_nr]) {
                fragments_completed[fragment_nr] = true;
                ++num_fragments_completed;
                request_registry.remove(fragment_nr); // Remove requests for collected fragment
            }
        }



        // Base session information //

        inline bool fragment_completed(size_t fragment_nr) {
            return fragments_completed[fragment_nr];
        }

        inline const auto& get_fragments_completed() {
            return fragments_completed;
        }

        inline size_t get_num_fragments() {
            return num_fragments;
        }

        inline bool download_completed() const {
            return num_fragments == num_fragments_completed;
        }



        // const member access methods //

        inline const auto& get_hashtable() const { return htable; }

        inline const auto& get_metadata() const { return metadata; }

        inline auto& get_handler() {
            return fragmentHandler;
        }
        inline const auto& get_conn() const { return recv_conn; }

        inline void set_address(Address& address) { own_address = address; }

        inline const auto& get_address() const { return own_address; }

        inline const auto& get_peer_registry() const { return peer_registry; }
        inline const auto& get_request_registry() const { return request_registry; }

        inline const auto& get_peertable() const { return ptable; }



        // Peertable-related forwarding functions //

        inline bool add_peer(const Address& a) { return ptable.add_ip(a); }
        inline bool add_peer(ConnectionType type, const std::string& ip, uint16_t port) { return ptable.add_ip(type, ip, port); }  
        inline void add_peers(const IPTable& peertable) {ptable.merge(peertable); }

        inline void set_peers(IPTable&& peertable) { ptable = std::move(peertable); }

        // We should only call this if we found that a certain peer is dead
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
        inline bool has_peer(const std::string& ip) {
            return ptable.contains(ip);
        }

        inline size_t peers_amount() { return ptable.size(); }



        // Peer Registry-related forwarding functions //

        inline void mark_registered_peer(const std::string& ip) {
            peer_registry.mark(ip);
        }
        inline void mark_registered_peer(const Address& address) {
            peer_registry.mark(address.ip);
        }

        inline void register_peer(const Address& address, const std::vector<bool>& fragments_completed) {
            peer_registry.add(address, fragments_completed);
        }
        inline void register_peer(ConnectionType type, const std::string ip, uint16_t port, const std::vector<bool>& fragments_completed) {
            register_peer(Address(type, ip, port), fragments_completed);
        }

        inline bool has_registered_peer(const std::string& ip) {
            return peer_registry.contains(ip);
        }

        inline void update_registered_peer_fragments(const std::string& ip, std::vector<bool>&& fragments_completed) {
            peer_registry.update_peer_fragments(ip, std::move(fragments_completed));
        }


        // Request Registry-related forwarding functions //

        inline void deregister_peer(const std::string& ip) {
            peer_registry.remove(ip);
        }
        inline void register_request(size_t fragment_nr, const Address& address) {
            request_registry.add(fragment_nr, address);
        }
    };
}
#endif