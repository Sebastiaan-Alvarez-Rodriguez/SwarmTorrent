#include <cstring>
#include <string>
#include <memory>

#include "tracker.h"
#include "shared/connection/message/tracker/message.h"
#include "shared/connection/connection.h"


/* '''Note: ''' Protocol in this file is defined in /PEER2TRACKER.md */

// Quickly send a trackermessage request, with the body of the message containing the torrent hash
static bool send_request(std::unique_ptr<ClientConnection>& connection, std::string torrent_hash, message::TrackerMessage::Tag tag) {
    
    const auto m_size = torrent_hash.size();
    uint8_t* const data = (uint8_t*) malloc(sizeof(message::TrackerMessage::Header)+m_size);
    ((message::TrackerMessage::Header*) data)[0] = message::TrackerMessage::fromType(tag).withSize(m_size).header;
    memcpy(data + sizeof(message::TrackerMessage::Header), torrent_hash.c_str(), m_size);

    bool val = connection->sendmsg(data, sizeof(message::TrackerMessage::Header)+m_size);
    free(data);
    return val;
    // TODO: we should no longer assume the data is always received at some point.
    // Not important for now. Implement in calling functions?
}

bool connections::tracker::subscribe(std::unique_ptr<ClientConnection>& connection, std::string torrent_hash) {
    return send_request(connection, torrent_hash, message::TrackerMessage::Tag::SUBSCRIBE);
}

bool connections::tracker::unsubscribe(std::unique_ptr<ClientConnection>& connection, std::string torrent_hash) {
    return send_request(connection, torrent_hash, message::TrackerMessage::Tag::UNSUBSCRIBE);
}

bool connections::tracker::retrieve(std::unique_ptr<ClientConnection>& connection, std::string torrent_hash, IPTable& peertable) {
    if (!send_request(connection, torrent_hash, message::TrackerMessage::Tag::RECEIVE))
        return false;

    //TODO: Receive peertable        
    return true;
}