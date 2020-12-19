#include <cstring>
#include <string>
#include <memory>

#include "shared/connection/message/tracker/message.h"
#include "shared/connection/connection.h"
#include "shared/connection/protocol/connections.h"
#include "connections.h"


/* '''Note:''' Protocol in this file is defined in /PEER2TRACKER.md */

// Prepares a simple tracker message with given tag, and already inserts the torrent hash into the message body.
// extra_body_length bytes will be appended to the message
static uint8_t* prepare_request(const std::string& torrent_hash, message::tracker::Tag tag, size_t extra_body_length) {
    const auto m_size = torrent_hash.size();
    uint8_t* const data = (uint8_t*) malloc(message::tracker::bytesize()+m_size+extra_body_length);
    uint8_t* ptr = data;

    message::tracker::from(m_size+extra_body_length, tag).write(ptr);

    ptr += message::tracker::bytesize();
    memcpy(ptr, torrent_hash.c_str(), m_size);
    return data;
}

static uint8_t* prepare_request(const std::string& torrent_hash, message::tracker::Tag tag) {
    return prepare_request(torrent_hash, tag, 0);
}

// Quickly send a tracker request, with the body of the message containing the torrent hash
static bool send_request(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, message::tracker::Tag tag) {
    uint8_t* const data = prepare_request(torrent_hash, tag);
    bool val = connection->sendmsg(data, message::tracker::bytesize()+torrent_hash.size());
    free(data);
    return val;
}

bool connections::tracker::test(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::TEST);
}

bool connections::tracker::send::make_torrent(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::MAKE_TORRENT);
}

bool connections::tracker::send::receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::RECEIVE);
}

// Message remote tracker to register
// peermessage (tag REGISTER)
// hash (string)
// port to register (uint16_t)
bool connections::tracker::send::register_self(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port) {
    uint8_t* const data = prepare_request(torrent_hash, message::tracker::REGISTER, sizeof(uint16_t));

    uint8_t* ptr = data+message::tracker::bytesize()+torrent_hash.size();
    *(uint16_t* ) ptr = port;

    bool val = connection->sendmsg(data, message::tracker::bytesize()+torrent_hash.size()+sizeof(uint16_t));
    free(data);
    return val;
}



bool connections::tracker::recv::receive(std::unique_ptr<ClientConnection>& connection, IPTable& peertable, Address& own_address, uint16_t sourcePort) {
    const auto& header = message::tracker::recv(connection);

    uint8_t* const data = (uint8_t*) malloc(header.size);
    connection->recvmsg(data, header.size);

    // Body of the message only contains a number of addresses.
    // Each address is const-size, so we can get amount of addresses simply by doing below.
    // First address is own address
    const size_t amount = ((header.size - message::tracker::bytesize()) / Address::size()) - 1;
    const uint8_t* reader = data + message::tracker::bytesize();
    reader = own_address.read_buffer(reader);
    own_address.port = sourcePort;

    for (size_t x = 0; x < amount; ++x) {
        Address a;
        reader = a.read_buffer(reader);
        if (!peertable.add(a)) {
            return false;
            free(data);
        }
    }
    free(data);

    peertable.remove(own_address);
    return true;
}