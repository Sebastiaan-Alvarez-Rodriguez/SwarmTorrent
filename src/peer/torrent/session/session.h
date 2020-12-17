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

#include "shared/util/hash/hasher.h"
#include "shared/util/fs/fs.h"

namespace peer::torrent {
    class Session {
    protected:
        const HashTable htable;
        const TorrentMetadata metadata;
        FragmentHandler fragmentHandler;

        Address own_address;

        peer::torrent::PeerRegistry peer_registry;
        peer::torrent::RequestRegistry request_registry;
        IPTable ptable; // table containing peers we might join. For joined peers, see [[peer_registry]]
        const IPTable ttable; // table containing trackers, as specified by the TorrentFile

        size_t num_fragments_completed = 0;
        std::vector<bool> fragments_completed;

    public:
        const size_t num_fragments;

        // The port on which this peer is listening
        const uint16_t registered_port;

        // Simple random number generator to use during this session.
        // Initialized such that different peers generate different numbers
        rnd::RandomGenerator<size_t> rand;

        /**
         * Constructs a session.
         *
         * @param tf torrentfile to use for torrenting
         * @param workpath path to where we load/store fragments
         * @param registered_port port on which this peer is listening
         *
         * '''Note:''' Ownership of `recv_conn` is passed to this session upon construction.
         *             Connection is closed when the session is deconstructed.
         */
        inline explicit Session(const TorrentFile& tf, const std::string& workpath, uint16_t registered_port) : htable(tf.getHashTable()), metadata(tf.getMetadata()), fragmentHandler(metadata, workpath + metadata.name), ttable(tf.getTrackerTable()), fragments_completed(metadata.get_num_fragments(), false), num_fragments(metadata.get_num_fragments()), registered_port(registered_port), rand(std::move(std::random_device())) {
            // if (fs::is_file(workpath+metadata.name)) {
            std::cerr << "Checking out fragments...\n";
            // We check if the hash is correct for each fragment of the file.
            // For all matches, we set the corresponding completed-bit to true
            const auto filesize = fs::file_size(workpath + metadata.name);
            if (filesize != 0) {
                const auto fragsize = fragmentHandler.fragment_size;
                const auto frags_to_read = ((filesize-1) / fragsize)+1;
                for (size_t x = 0; x < frags_to_read; ++x) {
                    uint8_t* data;
                    unsigned size;
                    if (fragmentHandler.read(x, data, size)) {
                        std::string fragment_hash;
                        hash::sha256(fragment_hash, data, size);
                        if (!htable.check_hash(x, fragment_hash)) {// Hash mismatch, wrong data
                            continue;
                        } else {
                            fragments_completed[x] = true; 
                            ++num_fragments_completed;      
                        }
                    }
                }
            }
            std::cerr << "Reading complete. " << num_fragments_completed << '/' << num_fragments << " OK." << (num_fragments_completed == num_fragments ? " Download completed.\n" : "\n");
            // }
        }

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

        inline void set_address(Address& address) { own_address = address; }

        inline const auto& get_address() const { return own_address; }

        inline const auto& get_peer_registry() const { return peer_registry; }
        inline const auto& get_request_registry() const { return request_registry; }

        inline const auto& get_peertable() const { return ptable; }

        inline const auto& get_trackertable() const { return ttable; }


        // Peertable-related forwarding functions //

        inline bool add_peer(const Address& a) { return ptable.add(a); }
        inline void add_peers(const IPTable& peertable) {ptable.merge(peertable); }

        inline void set_peers(IPTable&& peertable) { ptable = std::move(peertable); }

        // We should only call this if we found that a certain peer is dead
        inline bool remove_peer(const std::string& ip, uint16_t port) {
            Address a;
            if (!ptable.get_addr(ip, port, a))
                return false;    
            ptable.remove(a);
            return true;
        }
        inline bool has_peer(const Address& address) {
            return ptable.contains(address);
        }

        inline size_t peers_amount() { return ptable.size(); }



        // Peer Registry-related forwarding functions //

        inline void mark_registered_peer(const Address& address) {
            peer_registry.mark(address);
        }

        inline void report_registered_peer(const Address& address) {
            peer_registry.report(address);
        }

        inline void register_peer(Address&& address, const std::vector<bool>& fragments_completed) {
            peer_registry.add(address, fragments_completed);
        }
        inline void register_peer(const Address& address, const std::vector<bool>& fragments_completed) {
            peer_registry.add(address, fragments_completed);
        }

        inline void deregister_peer(const Address& address) {
            peer_registry.remove(address);
        }

        inline bool has_registered_peer(const Address& address) {
            return peer_registry.contains(address);
        }

        inline void update_registered_peer_fragments(const Address& address, std::vector<bool>&& fragments_completed) {
            peer_registry.update_peer_fragments(address, std::move(fragments_completed));
        }

        inline void peer_registry_gc() {
            peer_registry.gc();
        }

        inline auto get_peers_for(size_t fragment_nr) const {
            return peer_registry.get_peers_for(fragment_nr);
        }


        // Request Registry-related forwarding functions //

        inline void register_request(size_t fragment_nr, const Address& address) {
            request_registry.add(fragment_nr, address);
        }
        inline void request_registry_gc() {
            request_registry.gc();
        }
    };
}
#endif