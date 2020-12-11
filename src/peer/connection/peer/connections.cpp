#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include "peer/connection/message/peer/message.h"
#include "shared/connection/message/message.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "connections.h"


// Prepares a request, by allocating a buffer for given datasize, starting with a message::peer::Header of given type.
// '''Note:''' size of a message::peer::Header is automatically appended to the datasize!
// Returns pointer to start of buffer.
static inline uint8_t* prepare_peer_message(size_t datasize, message::peer::Tag tag) {
    uint8_t* const data = (uint8_t*) malloc(sizeof(message::peer::Header)+datasize);
    *((message::peer::Header*) data) = message::peer::from(datasize, tag);
    return data;
}

// Same as above function, but then for standard messages
static inline uint8_t* prepare_standard_message(size_t datasize, message::standard::Tag tag) {
    uint8_t* const data = (uint8_t*) malloc(sizeof(message::standard::Header)+datasize);
    *((message::standard::Header*) data) = message::standard::from(datasize, tag);
    return data;
}

bool connections::peer::test(__attribute__ ((unused)) std::unique_ptr<ClientConnection>& connection) {
    std::cerr << "P2P testing function ready for implementation\n";
    return false;
}

bool connections::peer::send::join(std::unique_ptr<ClientConnection>& connection, uint16_t port, const std::string& torrent_hash) {
    auto datasize = sizeof(uint16_t)+torrent_hash.size();
    uint8_t* const data = prepare_peer_message(datasize, message::peer::JOIN);
    uint8_t* const ptr = data + sizeof(message::peer::Header);
    *(uint16_t*) ptr = port;
    memcpy(ptr+sizeof(uint16_t), torrent_hash.c_str(), torrent_hash.size());
    bool val = connection->sendmsg(data, sizeof(message::peer::Header)+datasize);
    free(data);
    return val;
}

bool connections::peer::send::leave(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash) {
    auto datasize = torrent_hash.size();
    uint8_t* const data = prepare_peer_message(datasize, message::peer::LEAVE);
    uint8_t* const ptr = data + sizeof(message::peer::Header);
    memcpy(ptr, torrent_hash.c_str(), datasize);
    bool val = connection->sendmsg(data, sizeof(message::peer::Header)+datasize);
    free(data);
    return val;
}

bool connections::peer::send::data_req(std::unique_ptr<ClientConnection>& connection, size_t fragment_nr) {
    auto datasize = sizeof(size_t);
    uint8_t* const data = prepare_peer_message(datasize, message::peer::DATA_REQ);
    uint8_t* const ptr = data + sizeof(message::peer::Header);
    *((size_t*) ptr) = fragment_nr;
    bool val = connection->sendmsg(data, sizeof(message::peer::Header)+datasize);
    free(data);
    return val;
}

bool connections::peer::send::data_reply_fast(std::unique_ptr<ClientConnection>& connection, uint8_t* data, unsigned size) {
    *((message::peer::Header*) data) = message::peer::from_r(size, message::peer::DATA_REPLY);
    bool val = connection->sendmsg(data, size);
    free(data);
    return val;
}

bool connections::peer::recv::join(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port) {
    port = *(uint16_t*) (data+sizeof(message::peer::Header));
    const size_t hash_size = size-sizeof(uint16_t)-sizeof(message::peer::Header);

    hash.resize(hash_size);
    memcpy((char*) hash.data(), (char*)(data+sizeof(message::peer::Header)), hash_size);
    return true;
}
