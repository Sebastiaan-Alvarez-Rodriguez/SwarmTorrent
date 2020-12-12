#include <cstring>
#include <string>
#include <memory>

#include "shared/connection/message/tracker/message.h"
#include "shared/connection/connection.h"
#include "connections.h"


/* '''Note: ''' Protocol in this file is defined in /PEER2TRACKER.md */

// Quickly send a tracker request, with the body of the message containing the torrent hash
static bool send_request(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, message::tracker::Tag tag) {
    const auto m_size = torrent_hash.size();
    uint8_t* const data = (uint8_t*) malloc(sizeof(message::tracker::Header)+m_size);
    uint8_t* ptr = data;
    *((message::tracker::Header*) data) = message::tracker::from(m_size, tag);

    ptr += sizeof(message::tracker::Header);
    memcpy(ptr, torrent_hash.c_str(), m_size);
    bool val = connection->sendmsg(data, sizeof(message::tracker::Header)+m_size);
    free(data);
    return val;
}

bool connections::tracker::test(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::TEST);
}

bool connections::tracker::send::make_torrent(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::MAKE_TORRENT);
}

bool connections::tracker::send::register_self(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port) {
    const auto m_size = sizeof(uint16_t)+torrent_hash.size();
    uint8_t* const data = (uint8_t*) malloc(sizeof(message::tracker::Header)+m_size);
    uint8_t* ptr = data;
    *((message::tracker::Header*) data) = message::tracker::from(m_size, message::tracker::REGISTER);
    ptr += sizeof(message::tracker::Header);
    *((uint16_t* ) ptr) = port;
    ptr += sizeof(uint16_t);
    memcpy(ptr, torrent_hash.c_str(), m_size-sizeof(uint16_t));
    bool val = connection->sendmsg(data, sizeof(message::tracker::Header)+m_size);
    free(data);
    return val;
}


bool connections::tracker::recv::receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable, Address& own_address) {
    if (!send_request(connection, torrent_hash, message::tracker::RECEIVE))
        return false;

    message::standard::Header header;
    connection->peekmsg((uint8_t*)&header, sizeof(header));

    uint8_t* const table_buffer = (uint8_t*)malloc(header.size);
    connection->recvmsg(table_buffer, header.size);

    // Body of the message only contains a number of addresses.
    // Each address is const-size, so we can get amount of addresses simply by doing below.
    // First address is own address
    const size_t amount = ((header.size - sizeof(header)) / Address::size()) - Address::size();
    const uint8_t* reader = table_buffer + sizeof(header);
    reader = own_address.read_buffer(reader);
    for (size_t x = 0; x < amount; ++x) {
        Address a;
        reader = a.read_buffer(reader);
        if (a == own_address)
            continue;
        if (!peertable.add_ip(a))
            return false;
    }
    free(table_buffer);
    return true;
}