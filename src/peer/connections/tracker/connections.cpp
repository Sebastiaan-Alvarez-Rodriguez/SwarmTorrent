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

    //TODO @Mariska: place code here to receive the peertable and store in 'peertable'
    return true;
}