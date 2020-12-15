#ifndef TP_CONNECTION_PEER_PEER_H
#define TP_CONNECTION_PEER_PEER_H

#include <string>
#include <memory>

#include "shared/connection/connection.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "shared/torrent/ipTable/ipTable.h"

namespace connections::peer {
    // Send a simple test message to peer
    bool test(std::unique_ptr<ClientConnection> &connection);

    namespace send {
        /**
         * Request to join remote peer for given torrent hash.
         * Remote peer will send all future communication to given port.
         * We deliver our list of completed fragments.
         * '''Note:''' other end will first send an accept/reject using current established connection.
         */
        bool join(std::unique_ptr<ClientConnection>& connection, uint16_t port, const std::string& torrent_hash, const std::vector<bool>& fragments_completed);

        // Sends positive answer to join request
        bool join_reply(const std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, const std::vector<bool>& fragments_completed);
        
        // Tell peer that we no longer cooperate with them
        bool leave(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port);

        // Request peer to send us fragment with given fragment number
        bool data_req(std::unique_ptr<ClientConnection>& connection, size_t fragment_nr);

        // Reply to peer with a REJECT, and a bool vector of currently owned fragments
        bool data_rej(std::unique_ptr<ClientConnection>& connection, const std::vector<bool>& fragments_completed);

        /** 
         * Reply to peer with a data fragment. Call this function only for positive replies to a DATA_REQ
         *
         * '''Warning:''' assumes that the data buffer contains sizeof(message::peer::Header)+sizeof(size_t)
         *                leading bytes of free space
         * '''Note:''' This function frees the data after transmission.
         */
        bool data_reply_fast(const std::unique_ptr<ClientConnection>& connection, size_t fragment_nr, uint8_t* data, unsigned size);

        /**
         * Returns whether peer is dead or not
         *
         * '''Warning:''' assumes that the connection has a timeout set, or is nonblocking,
         *                otherwise it may not return in a finite time
         */
        bool inquire(const std::unique_ptr<ClientConnection>& connection);

        // Send an AVAILABILITY request to a fellow peer
        bool availability(const std::unique_ptr<ClientConnection>& connection, const std::string& hash, const std::vector<bool>& fragments_completed);
    
        // Reply to an AVAILABILITY request from a fellow peer
        bool availability_reply(const std::unique_ptr<ClientConnection>& connection, const std::vector<bool>& fragments_completed);
    }

    namespace recv {
        // Get arguments for JOIN messages from raw buffer
        bool join(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port, std::vector<bool>& fragments_completed);

        // Get arguments for JOIN replies from raw buffer. Only call this for positive JOIN replies
        bool join_reply(const uint8_t* const data, size_t size, std::string& hash, std::vector<bool>& fragments_completed);

        // Get arguments for LEAVE messages from raw buffer 
        bool leave(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port);

        // Get arguments for DATA_REQ messages from raw buffer
        bool data_req(const uint8_t* const data, size_t size, size_t& fragment_nr);

        // Get arguments for DATA_REPLY messages from raw buffer
        bool data_reply(const uint8_t* const data, size_t size, size_t& fragment_nr, uint8_t*& fragment_data);

        // Get arguments for AVAILABILITY messages from raw buffer
        bool availability(const uint8_t* const data, size_t size, std::string& hash, std::vector<bool>& fragments_completed);

        // Get arguments for AVAILABILITY messages from raw buffer
        bool availability_reply(const uint8_t* const data, size_t size, std::vector<bool>& fragments_completed);
    }
}
#endif