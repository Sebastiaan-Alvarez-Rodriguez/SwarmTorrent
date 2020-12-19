#ifndef PEER_TORRENT_SESSION_H
#define PEER_TORRENT_SESSION_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <random>
#include <shared_mutex>

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

        Address own_address;

        peer::torrent::PeerRegistry peer_registry;
        mutable std::shared_mutex peer_registry_mutex;
        peer::torrent::RequestRegistry request_registry;
        mutable std::shared_mutex request_registry_mutex;

        IPTable ptable; // table containing peers we might join. For joined peers, see [[peer_registry]]
        mutable std::shared_mutex ptable_mutex;

        size_t num_fragments_completed = 0;
        std::vector<bool> fragments_completed;
        mutable std::shared_mutex fragments_completed_mutex;

    public:
        const HashTable hashtable;
        const TorrentMetadata metadata;
        const IPTable trackertable; // table containing trackers, as specified by the TorrentFile
        
        const std::string workpath;
        const size_t num_fragments;

        // The port on which this peer is listening
        const uint16_t registered_port;

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
        inline explicit Session(const TorrentFile& tf, const std::string& workpath, uint16_t registered_port) : fragments_completed(tf.getMetadata().get_num_fragments(), false), hashtable(tf.getHashTable()), metadata(tf.getMetadata()), trackertable(tf.getTrackerTable()), num_fragments(metadata.get_num_fragments()), registered_port(registered_port) {
            // if (fs::is_file(workpath+metadata.name)) {
            std::cerr << "Checking out " << fragments_completed.size() << " fragments...\n";
            // We check if the hash is correct for each fragment of the file.
            // For all matches, we set the corresponding completed-bit to true
            const auto filesize = fs::file_size(workpath + metadata.name);
            if (filesize == metadata.size) {
                FragmentHandler fragmentHandler(metadata, workpath + metadata.name);
                for (size_t x = 0; x < fragments_completed.size(); ++x) {
                    unsigned size;
                    uint8_t* data = fragmentHandler.read(x, size);
                    if (data != nullptr) {
                        std::string fragment_hash;
                        hash::sha256(fragment_hash, data, size);
                        if (!hashtable.check_hash(x, fragment_hash)) {// Hash mismatch, wrong data
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
            std::shared_lock lock(fragments_completed_mutex);
            return fragments_completed[fragment_nr];
        }

        inline const auto& get_fragments_completed() { return fragments_completed; }

        // Returns a valid copy of the fragments vector to operate on
        inline const auto get_fragments_completed_copy() {
            std::shared_lock lock(fragments_completed_mutex);
            return fragments_completed;
        }

        inline bool download_completed() const {
            std::shared_lock lock(fragments_completed_mutex);
            return num_fragments == num_fragments_completed;
        }



        // basic member access methods //

        inline const auto& get_hashtable() const { return hashtable; }

        inline const auto& get_metadata() const { return metadata; }

        /**
         * Sets address of ourselves
         *
         * '''Warning:''' We do not use thread-safety here. We assume you call this before booting threads.
         */
        inline void set_address(Address& address) { own_address = address; }

        /**
         * Gets address of ourselves. Undefined behaviour if [[set_address()]] is not called first.
         *
         * '''Warning:''' We do not use thread-safety here, because we assume you call [[set_address()]] before getting this in separate threads.
         */
        inline const auto& get_address() const { return own_address; }

        inline const auto& get_peer_registry() const { return peer_registry; }

        // Returns a copy of the peer registry, thread-safe
        inline const auto get_peer_registry_copy() const {
            return peer_registry.copy();
        }

        inline const auto& get_request_registry() const { return request_registry; }

        inline const auto get_request_registry_copy() const {
            return request_registry.copy();
        }

        inline const auto& get_peertable() const { return ptable; }

        inline const auto get_peertable_copy() const {
            std::shared_lock lock(ptable_mutex);
            return ptable.copy();
        }

        inline const auto& get_trackertable() const { return trackertable; }


        // Peertable-related forwarding functions //

        inline auto get_peertable_lock_read() {
            std::shared_lock lock(ptable_mutex);
            return lock;
        }

        inline bool add_peer(const Address& a) {
            std::unique_lock lock(ptable_mutex);
            return ptable.add(a); 
        }
        inline void add_peers(const IPTable& peertable) {
            std::unique_lock lock(ptable_mutex);
            ptable.merge(peertable); 
        }

        inline void set_peers(IPTable&& peertable) {
            std::unique_lock lock(ptable_mutex);
            ptable = std::move(peertable);
        }

        // We should only call this if we found that a certain peer is dead
        inline bool remove_peer(const std::string& ip, uint16_t port) {
            Address a;
            std::shared_lock readlock(ptable_mutex);
            if (!ptable.get_addr(ip, port, a))
                return false;
            readlock.unlock();

            std::unique_lock writelock(ptable_mutex);
            ptable.remove(a);
            return true;
        }

        inline bool has_peer(const Address& address) {
            std::shared_lock lock(ptable_mutex);
            return ptable.contains(address);
        }
        inline size_t num_known_peers() const {
            std::shared_lock lock(ptable_mutex);
            return ptable.size();
        }



        // Peer Registry-related forwarding functions //

        inline void mark_registered_peer(const Address& address) {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.mark(address);
        }

        inline void report_registered_peer(const Address& address) {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.report(address);
        }

        inline void register_peer(Address&& address, const std::vector<bool>& fragments_completed) {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.add(address, fragments_completed);
        }
        inline void register_peer(const Address& address, const std::vector<bool>& fragments_completed) {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.add(address, fragments_completed);
        }

        inline void deregister_peer(const Address& address) {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.remove(address);
        }

        inline bool has_registered_peer(const Address& address) {
            std::shared_lock lock(peer_registry_mutex);
            return peer_registry.contains(address);
        }

        inline void update_registered_peer_fragments(const Address& address, std::vector<bool>&& fragments_completed) {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.update_peer_fragments(address, std::move(fragments_completed));
        }

        inline void peer_registry_gc() {
            std::unique_lock lock(peer_registry_mutex);
            peer_registry.gc();
        }

        inline auto get_peers_for(size_t fragment_nr) const {
            std::shared_lock lock(peer_registry_mutex);
            return peer_registry.get_peers_for(fragment_nr);
        }

        inline size_t num_registered_peers() {
            std::shared_lock lock(peer_registry_mutex);
            return peer_registry.size();
        }

        // Request Registry-related forwarding functions //

        inline void register_request(size_t fragment_nr, const Address& address) {
            std::unique_lock lock(request_registry_mutex);
            request_registry.add(fragment_nr, address);
        }
        inline void request_registry_gc() {
            std::unique_lock lock(request_registry_mutex);
            request_registry.gc();
        }
        inline size_t num_requests() {
            std::shared_lock lock(request_registry_mutex);
            return request_registry.size();
        }
    };
}
#endif