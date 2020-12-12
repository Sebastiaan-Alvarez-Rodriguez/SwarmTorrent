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
         * Request to join peer for given torrent hash.
         * Peer will send all future communication to given port.
         * '''Note:''' other end will first send an accept/reject using current established connection.
         */
        bool join(std::unique_ptr<ClientConnection> &connection, uint16_t port, const std::string &torrent_hash);

        // Tell peer that we no longer cooperate with them
        bool leave(std::unique_ptr<ClientConnection> &connection, const std::string &torrent_hash);

        // Request peer to send us fragment with given fragment number
        bool data_req(std::unique_ptr<ClientConnection> &connection, size_t fragment_nr);

        /** 
         * Reply to peer with a data fragment
         *
         * '''Warning:''' assumes that the data buffer contains sizeof(message::peer::Header)+sizeof(size_t)
         *                leading bytes of free space
         * '''Note:''' This function frees the data after transmission.
         */
        bool data_reply_fast(const std::unique_ptr<ClientConnection>& connection, size_t fragment_nr, uint8_t* data, unsigned size);
    }

    namespace recv {
        // Get arguments for EXCHANGE_REQ messages from raw buffer
        bool join(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port);

        // Get arguments for EXCHANGE_CLOSE messages from raw buffer 
        inline bool leave(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port) {
            return join(data, size, hash, port); // We just call join implementation, as it has same data scheme
        }

        // Get arguments for DATA_REQ messages from raw buffer
        bool data_req(const uint8_t* const data, size_t size, size_t& fragment_nr);


        bool data_reply(const uint8_t* const data, size_t size, size_t& fragment_nr, uint8_t*& fragment_data);
    }
}
#endif