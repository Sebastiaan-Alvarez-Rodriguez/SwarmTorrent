#include <cstdint>
#include <memory>
#include <iostream>


#include "peer/connection/peer/connections.h"
#include "peer/connection/message/peer/message.h"
#include "peer/torrent/session/session.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/message.h"
#include "shared/connection/meta/type.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "shared/util/hash/hasher.h"
#include "shared/util/print.h"


#include "pipe_ops.h"

void peer::pipeline::join(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    //TODO: For now always accept join requests. In the future, add reasonable limit on joined peers
    uint16_t req_port;
    std::string hash;
    connections::peer::recv::join(data, size, hash, req_port);
    std::cerr << "Got an JOIN (hash=" << hash << ", req_port=" << req_port << ")\n";
    if (session.get_metadata().content_hash != hash) { // Torrent mismatch, Reject
        std::cerr << "Above hash mismatched with our own (" << session.get_metadata().content_hash <<"), rejected.\n";
        message::standard::send(connection, message::standard::REJECT);
        return;
    }
    session.add_peer(connection->get_type(), connection->getAddress(), req_port);
    message::standard::send(connection, message::standard::OK);
}

void peer::pipeline::leave(torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    uint16_t req_port;
    std::string hash;
    connections::peer::recv::leave(data, size, hash, req_port);
    std::cerr << "Got a lEAVE (hash=" << hash << ", req_port=" << req_port << ")\n";
    if (session.get_metadata().content_hash != hash) // Torrent mismatch, ignore
        return;
    session.remove_peer(connection->getAddress(), req_port);
}

// Handles DATA_REQs. Closes incoming connection, reads fragment from storage, sends data to registered port for given ip.
void peer::pipeline::data_req(torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    auto connected_ip = connection->getAddress();
    
    if (!session.has_registered_peer(connected_ip)) { //Reject data requests from unknown entities
        message::standard::send(connection, message::standard::REJECT);
        return;
    }
    size_t fragment_nr;
    connections::peer::recv::data_req(data, size, fragment_nr);
    if (fragment_nr > session.get_num_fragments()) {
        std::cerr << "Cannot get fragment number " << fragment_nr << ", only " << session.get_num_fragments() << " fragments exist!\n";
        message::standard::send(connection, message::standard::REJECT); //TODO: Maybe pick another flag? This flag suggests other end should disconnect
        return;
    }
    connection.reset(); // Closes the connection

    auto& handler = session.get_handler();
    uint8_t* diskdata;
    unsigned data_size;
    if (!handler.read_with_leading(fragment_nr, (uint8_t*) &diskdata, data_size, sizeof(message::peer::Header)+sizeof(size_t))) {
        std::cerr << "There was a problem reading fragment " << fragment_nr << " from disk\n";
        free(diskdata);
        return;
    }

    Address a;
    session.get_peertable().get_addr(connected_ip, a);
    auto target_conn = TCPClientConnection::Factory::from(a.type.n_type).withAddress(a.ip).withDestinationPort(a.port).create();
    if (target_conn->get_state() != ClientConnection::READY) {
        std::cerr << print::YELLOW << "[WARN] Could not initialize connection to peer: " << print::CLEAR; target_conn->print(std::cerr);std::cerr << '\n';
        free(diskdata);
        return;
    }
    if (!target_conn->doConnect()) {
        std::cerr << "Could not connect to peer ";target_conn->print(std::cerr);std::cerr << '\n';
        free(diskdata);
        return;
    }
    if (!connections::peer::send::data_reply_fast(target_conn, fragment_nr, diskdata, data_size)) {
        std::cerr << "Could not send data to peer. Hangup? Some other problem?\n";
    } else {
        std::cerr << "Sent fragment nr " << fragment_nr << ", size=" << data_size << " bytes to peer "; target_conn->print(std::cerr); std::cerr << '\n';
    }
    free(diskdata);
}

void peer::pipeline::data_reply(torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    //For incoming connections:
    // 1. Check if in session
    // 2. Maybe (send on other side and) check if content hash equals our content hash?
    // 3. Check if we already own the data
    // 4. Maybe check if we have asked for that data from this particular node in the registry
    // 5. Check if data matches hash for that data
    // 6. Write to disk at right location.
    // 7. Mark object as completed when finished
    auto connected_ip = connection->getAddress();

    if (!session.has_registered_peer(connected_ip)) { //Reject data replies from unknown entities
        // TODO: Think a bit about this. Perhaps it is okay to accept data replies from unknowns, as long as their data matches the hash?
        message::standard::send(connection, message::standard::REJECT);
        return;
    }
    connection.reset(); // Closes the connection

    size_t fragment_nr;
    uint8_t* fragment_data;
    connections::peer::recv::data_reply(data, size, fragment_nr, fragment_data);

    if (session.fragment_completed(fragment_nr)) // We already have this fragment
        return;

    const size_t fragment_size = size - sizeof(message::peer::Header) - sizeof(size_t);

    std::string fragment_hash;
    hash::sha256(fragment_hash, fragment_data, fragment_size);
    if (!session.get_hashtable().check_hash(fragment_nr, fragment_hash)) {// Hash mismatch, wrong data
        std::cerr << "There was a hash mismatch for fragment " << fragment_nr << '\n';
        return;
    }

    auto& handler = session.get_handler();
    if (!handler.write(fragment_nr, fragment_data, fragment_size)) { // Could not write data?
        std::cerr << "There was a problem writing fragment " << fragment_nr << " to disk\n";
        return;
    }
    session.mark(fragment_nr);
}
