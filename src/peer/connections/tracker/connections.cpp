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

bool connections::tracker::subscribe(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::Tag::SUBSCRIBE);
}

bool connections::tracker::unsubscribe(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    return send_request(connection, torrent_hash, message::tracker::Tag::UNSUBSCRIBE);
}

bool connections::tracker::receive(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable) {
    if (!send_request(connection, torrent_hash, message::tracker::Tag::RECEIVE))
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

bool connections::tracker::make_torrent(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, IPTable& peertable) {
    size_t buf_length = sizeof(message::tracker::Header) + 1 + torrent_hash.length() + 1 + sizeof(size_t) + peertable.size() * Address::size();
    uint8_t* const buff = (uint8_t*)malloc(buf_length);
    uint8_t* writer = buff;
    *((message::tracker::Header*) writer) = message::tracker::from(1 + torrent_hash.length() + 1 + sizeof(size_t) + peertable.size() * Address::size(), message::tracker::Tag::MAKE_TORRENT);
    writer += sizeof(message::tracker::Header);

    *((size_t*) writer) = torrent_hash.length();
    writer += sizeof(size_t);

    memcpy(writer, torrent_hash.data(), torrent_hash.length()+1);
    writer += torrent_hash.length()+1;

    *((size_t*) writer) = peertable.size();
    writer += sizeof(size_t);

    for (auto it = peertable.iterator_begin(); it != peertable.iterator_end(); ++it) 
        writer = (*it).second.write_buffer(writer);

    bool val = connection->sendmsg(buff, buf_length);
    free(buff);
    return val;
}