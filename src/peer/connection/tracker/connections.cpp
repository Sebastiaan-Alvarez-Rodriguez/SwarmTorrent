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

bool connections::tracker::make_torrent(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::MAKE_TORRENT);
}

bool connections::tracker::receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable) {
    if (!send_request(connection, torrent_hash, message::tracker::RECEIVE))
        return false;

    message::standard::Header header;
    connection->peekmsg((uint8_t*)&header, sizeof(header));
    size_t buf_length = header.size;
    uint8_t* const table_buffer = (uint8_t*)malloc(buf_length);
    connection->recvmsg(table_buffer, buf_length);
    const uint8_t* reader = table_buffer + sizeof(header);
    while (reader < table_buffer + buf_length) {
        Address a(ConnectionType(TransportType(), NetType()), "", 0);
        reader = a.read_buffer(reader);
        if (!peertable.add_ip(a))
            return false;
    }
    free(table_buffer);
    return true;
}

bool connections::tracker::register_self(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port) {
    const auto m_size = torrent_hash.size();
    uint8_t* const data = (uint8_t*) malloc(sizeof(message::tracker::Header)+sizeof(uint16_t)+m_size);
    uint8_t* ptr = data;
    *((message::tracker::Header*) data) = message::tracker::from(m_size, message::tracker::REGISTER);
    ptr += sizeof(message::tracker::Header);
    *((uint16_t* ) ptr) = port;
    ptr += sizeof(uint16_t);
    memcpy(ptr, torrent_hash.c_str(), m_size);
    bool val = connection->sendmsg(data, sizeof(message::tracker::Header)+m_size);
    free(data);
    return val;
}
