#include <cstdint>
#include <cstring>
#include <memory>
#include <iostream>


#include "peer/connection/protocol/peer/connections.h"
#include "peer/connection/message/peer/message.h"
#include "peer/torrent/session/session.h"
#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/connection/message/message.h"
#include "shared/connection/meta/type.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "shared/util/hash/hasher.h"
#include "shared/util/print.h"
#include "peer/torrent/defaults.h"

#include "pipe_ops.h"

void peer::pipeline::join(peer::torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    uint16_t req_port;
    std::string hash;
    std::vector<bool> fragments_completed;
    connections::peer::recv::join(data, size, hash, req_port, fragments_completed);

    //TODO: Remove below after testing
    size_t x = 0;
    for (const auto y : fragments_completed)
        if (y)
            ++x;

    std::cerr << "Got a JOIN (hash=" << hash << ", req_port=" << req_port << ", frags_completed="<<x<<'/'<<fragments_completed.size()<<", num_fragments="<<session.num_fragments<<")\n";
    if (fragments_completed.size() != session.num_fragments) {
        std::cerr << "ERRRRRRRRRRRROR: Remote did not do so well?\n";
        exit(1);
    }
    const auto addr = Address(connection->get_type(), connection->getAddress(), req_port);

    // Register peer as an existing peer
    session.add_peer(Address(connection->get_type(), connection->getAddress(), req_port));

    if (session.get_metadata().content_hash != hash) { // Torrent mismatch, Reject
        std::cerr << "Above hash mismatched with our own (" << session.get_metadata().content_hash <<"), rejected.\n";
        message::standard::send(connection, message::standard::REJECT);
        return;
    }

    if (session.get_peer_registry().size() > 4 * peer::torrent::defaults::prefered_group_size) {// too many peers
        std::cerr << "JOIN request rejected, too many group members already.\n";
        message::standard::send(connection, message::standard::REJECT);
        return;
    }

    // If we get here, we accept the JOIN request

    if (!connections::peer::send::join_reply(connection, hash, session.get_fragments_completed())) {
        std::cerr << "Could not send positive join reply to peer: "; connection->print(std::cerr); std::cerr << '\n';
        return;
    }

    std::cerr << print::CYAN << "We accepted the join request! " << print::CLEAR << "It has "<<x<<'/'<<fragments_completed.size()<<" fragments available"<<std::endl;
    // Register peer to our group!
    session.register_peer({connection->get_type(), connection->getAddress(), req_port}, fragments_completed);
}

void peer::pipeline::leave(peer::torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    uint16_t req_port;
    std::string hash;
    connections::peer::recv::leave(data, size, hash, req_port);
    std::cerr << "Got a lEAVE (hash=" << hash << ", req_port=" << req_port << ")\n";
    if (session.get_metadata().content_hash != hash) // Torrent mismatch, ignore
        return;
    session.remove_peer(connection->getAddress(), req_port); // Can call this safely: No effect if caller is not in our group
}

// Handles DATA_REQs. Closes incoming connection, reads fragment from storage, sends data to registered port for given ip.
void peer::pipeline::data_req(peer::torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    const auto connected_ip = connection->getAddress();
    
    uint16_t req_port;
    size_t fragment_nr;
    connections::peer::recv::data_req(data, size, req_port, fragment_nr);
    const auto addr = Address(connected_ip, req_port);
    std::cerr << "Received a DATA_REQ (port=" << req_port << ", fragment_nr=" << fragment_nr << ") ("<<addr.ip<<':'<<addr.port<<")\n";
    if (!session.has_registered_peer(addr)) { //Data requests from unknown entities produce only ERROR
        message::standard::send(connection, message::standard::ERROR);
        return;
    }

    if (fragment_nr > session.num_fragments) {
        std::cerr << "Cannot get fragment number " << fragment_nr << ", only " << session.num_fragments << " fragments exist!\n";
        connections::peer::send::data_rej(connection, session.get_fragments_completed());
        return;
    }
    if (!session.get_fragments_completed()[fragment_nr]) {
        std::cerr << "Cannot get fragment number " << fragment_nr << ", because we do not have it.\n";
        connections::peer::send::data_rej(connection, session.get_fragments_completed());
        return;
    }
    message::standard::send(connection, message::standard::OK);
    connection.reset(); // Closes the connection

    std::cerr << print::CYAN << "We accepted the DATA_REQ request!\n" << print::CLEAR << std::endl;

    auto& handler = session.get_handler();
    unsigned data_size;
    uint8_t* diskdata = handler.read_with_leading(fragment_nr, data_size, message::peer::bytesize()+sizeof(size_t));
    if (diskdata == nullptr) {
        std::cerr << "There was a problem reading fragment " << fragment_nr << " from disk\n";
        free(diskdata);
        return;
    }

    Address a;
    session.get_peertable().get_addr(connected_ip, req_port, a);
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
    std::cerr << "sending data reply to "; target_conn->print(std::cerr); std::cerr << ": fragment_nr="<<fragment_nr<<", data_size="<<data_size<<'\n';
    if (!connections::peer::send::data_reply_fast(target_conn, fragment_nr, diskdata, message::peer::bytesize()+sizeof(size_t)+data_size)) {
        std::cerr << "Could not send data to peer. Hangup? Some other problem?\n";
    } else {
        std::cerr << "Sent fragment nr " << fragment_nr << ", size=" << data_size << " bytes to peer "; target_conn->print(std::cerr); std::cerr << '\n';
    }
}

void peer::pipeline::data_reply(peer::torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    // 1. Check if in session
    // 2. Maybe (send on other side and) check if content hash equals our content hash?
    // 3. Check if we already own the data
    // 4. Maybe check if we have asked for that data from this particular node in the registry
    // 5. Check if data matches hash for that data
    // 6. Write to disk at right location.
    // 7. Mark object as completed when finished
    std::cerr << "Received a DATA_REPLY ";

    const auto connected_ip = connection->getAddress();
    const uint16_t port = connection->getSourcePort();
    // Note: Even if the address is not from our group, we still process the data.
    // After all: If the data matches the checksum, why would we not use it?

    connection.reset(); // Closes the connection

    size_t fragment_nr;
    uint8_t* fragment_data;
    connections::peer::recv::data_reply(data, size, fragment_nr, fragment_data);

    std::cerr << "(fragment_nr=" << fragment_nr << ") ("<<connected_ip<<':'<<port<<"). ";

    if (session.fragment_completed(fragment_nr)) // We already have this fragment
        return;

    const size_t fragment_size = size - message::peer::bytesize() - sizeof(size_t);

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
    std::cerr << "Marked fragment "<<fragment_nr<<" as complete." << std::endl;
    session.mark_fragment(fragment_nr);
}

void peer::pipeline::local_discovery(const peer::torrent::Session& session, const std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    std::string recv_hash;
    if (!connections::shared::recv::discovery_req(data, size, recv_hash))
        return;
    std::cerr << "Received a LOCAL_DISCOVERY_REQ (recv_hash="<<recv_hash<<", our hash="<<session.get_metadata().content_hash<<'\n';
    if (recv_hash != session.get_metadata().content_hash) // Hash mismatch
        return;

    std::cerr << "Sending LOCAL_DISCOVERY_REPLY. Note: We believe that our address="<<session.get_address().type<<':'<<session.get_address().ip<<':'<<session.get_address().port<<'\n';
    connections::shared::send::discovery_reply(connection, session.get_peertable(), recv_hash, session.get_address());
}

void peer::pipeline::availability(peer::torrent::Session& session, std::unique_ptr<ClientConnection>& connection, uint8_t* const data, size_t size) {
    std::cerr << "Got an AVAILABILITY request\n";
    uint16_t port;
    std::string recv_hash;
    std::vector<bool> state;
    if (!connections::peer::recv::availability(data, size, port, recv_hash, state)) { // If message interpreting failed, just return
        std::cerr << "Could not interpret data\n";
        return;
    }

    if (!session.get_peer_registry().contains({connection->getAddress(), port})) { // If not in group, reject
        std::cerr << "Peer not in registry, rejected.\n";
        message::standard::send(connection, message::standard::REJECT);
        return;
    }

    if (recv_hash != session.get_metadata().content_hash) { // They asked for a hash that we don't use. Just return.
        std::cerr << "Hash mismatched with our own (ours=" << session.get_metadata().content_hash <<", theirs="<<recv_hash<<"), rejected.\n";
        return;
    }
    if (!connections::peer::send::availability_reply(connection, session.get_fragments_completed()))
        std::cerr << "Could not send reply!\n";
}