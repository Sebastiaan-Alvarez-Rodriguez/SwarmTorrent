#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include "peer/connection/message/peer/message.h"
#include "shared/connection/message/message.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "connections.h"


// Prepares a request, by allocating a buffer for given datasize, starting with a message::peer::Header of given type.
// '''Note:''' size of a message::peer::Header is automatically appended to the datasize. Also: caller has to free returned buffer.
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

bool connections::peer::send::join(std::unique_ptr<ClientConnection>& connection, uint16_t port, const std::string& torrent_hash, const std::vector<bool>& fragments_completed) {
    const size_t bufsize = fragments_completed.size();

    auto datasize = sizeof(uint16_t)+torrent_hash.size()+bufsize;
    uint8_t* const data = prepare_peer_message(datasize, message::peer::JOIN);
    uint8_t* writer = data + sizeof(message::peer::Header);

    *(uint16_t*) writer = port;
    writer += sizeof(uint16_t);

    *(size_t*) writer = torrent_hash.size();
    writer += sizeof(size_t);

    memcpy(writer, torrent_hash.c_str(), torrent_hash.size());
    writer += torrent_hash.size();

    // vector of bools is internally packed. However, getting the data like that is impossible in a clean way.
    // Have to use byte-wise sending instead of packed
    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    bool val = connection->sendmsg(data, sizeof(message::peer::Header)+datasize);
    free(data);
    return val;
}

bool connections::peer::send::join_reply(const std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, const std::vector<bool>& fragments_completed) {
    // vector of bools is internally packed. However, getting the data like that is impossible in a clean way.
    // Have to use byte-wise sending instead of packed
    const size_t bufsize = sizeof(size_t) + torrent_hash.size() + fragments_completed.size();
    uint8_t* const buf = (uint8_t*) malloc(sizeof(message::standard::Header)+bufsize);
    uint8_t* writer = buf;

    *(message::standard::Header*) writer = message::standard::from(bufsize, message::standard::OK);
    writer += sizeof(message::standard::Header);

    *(size_t*) writer = torrent_hash.size();
    writer += sizeof(size_t);

    memcpy(writer, torrent_hash.c_str(), torrent_hash.size());
    writer += torrent_hash.size();

    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    if (!connection->sendmsg(buf, sizeof(message::standard::Header)+bufsize)) {
        std::cerr << "Could not reply to peer "; connection->print(std::cerr); std::cerr << '\n';
        free(buf);
        return false;
    }
    free(buf);
    return true;
}

bool connections::peer::send::leave(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port) {
    auto datasize = sizeof(uint16_t)+torrent_hash.size();
    uint8_t* const data = prepare_peer_message(datasize, message::peer::LEAVE);
    uint8_t* writer = data + sizeof(message::peer::Header);

    *(uint16_t*) writer = port;
    writer += sizeof(uint16_t);

    memcpy(writer, torrent_hash.c_str(), datasize);

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

bool connections::peer::send::data_rej(std::unique_ptr<ClientConnection> &connection, const std::vector<bool>& fragments_completed) {
    const auto frag_size = fragments_completed.size();

    uint8_t* const data = (uint8_t*) malloc(sizeof(message::standard::Header)+frag_size);
    *((message::standard::Header*) data) = message::standard::from(frag_size, message::standard::REJECT);
    uint8_t* writer = data + sizeof(message::standard::Header);
    // vector of bools is internally packed. However, getting the data like that is impossible in a clean way.
    // Have to use byte-wise sending instead of packed
    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    bool val = connection->sendmsg(data, sizeof(message::standard::Header)+frag_size);
    free(data);
    return val;
}

bool connections::peer::send::data_reply_fast(const std::unique_ptr<ClientConnection>& connection, size_t fragment_nr, uint8_t* data, unsigned size) {
    *((message::peer::Header*) data) = message::peer::from_r(size, message::peer::DATA_REPLY);
    *((size_t*) (data+sizeof(message::peer::Header))) = fragment_nr;
    bool val = connection->sendmsg(data, size);
    free(data);
    return val;
}

bool connections::peer::send::inquire(const std::unique_ptr<ClientConnection>& connection) {
    message::peer::Header header = message::peer::from(message::peer::INQUIRE);
    return connection->sendmsg((uint8_t*) &header, sizeof(header));
}

// Message:
// Peer message header (tag=AVAILABILITY)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::send::availability(const std::unique_ptr<ClientConnection>& connection, const std::string& hash, const std::vector<bool>& fragments_completed) {
    const size_t bufsize = sizeof(size_t) + hash.size() + fragments_completed.size();
    uint8_t* const buf = prepare_peer_message(bufsize, message::peer::AVAILABILITY);
    uint8_t* writer = buf;

    // Peer message header
    writer += sizeof(message::standard::Header);

    // Size of hash
    *(size_t*) writer = hash.size();
    writer += sizeof(size_t);

    // Hash
    memcpy((char*) writer, (char*) hash.c_str(), hash.size());
    writer += hash.size();

    // Fragment vector
    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    if (!connection->sendmsg(buf, sizeof(message::peer::Header)+bufsize)) {
        std::cerr << "Could not reply to peer "; connection->print(std::cerr); std::cerr << '\n';
        free(buf);
        return false;
    }
    free(buf);
    return true;

}

// Message:
// Peer message header (tag=AVAILABILITY)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::send::availability_reply(const std::unique_ptr<ClientConnection>& connection, const std::vector<bool>& fragments_completed) {
    const size_t bufsize = fragments_completed.size();
    uint8_t* const buf = (uint8_t*) malloc(sizeof(message::standard::Header)+bufsize);
    uint8_t* writer = buf;

    *(message::standard::Header*) writer = message::standard::from(bufsize, message::standard::OK);
    writer += sizeof(message::standard::Header);

    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    if (!connection->sendmsg(buf, sizeof(message::standard::Header)+bufsize)) {
        std::cerr << "Could not reply to peer "; connection->print(std::cerr); std::cerr << '\n';
        free(buf);
        return false;
    }
    free(buf);
    return true;
}




bool connections::peer::recv::join(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data;
    reader += sizeof(message::peer::Header);

    port = *(uint16_t*) reader;
    reader += sizeof(uint16_t);

    size_t hash_size = *(size_t*) reader;
    reader += sizeof(size_t);

    hash.resize(hash_size);
    memcpy((char*) hash.data(), (char*) reader, hash_size);

    size_t remaining_size = size - sizeof(uint16_t) - sizeof(size_t) - hash_size;
    fragments_completed.resize(remaining_size);
    for (size_t x = 0; x < remaining_size; ++x) {
        fragments_completed[x] = *(bool*) reader;
        reader += sizeof(bool);
    }

    return true;
}

bool connections::peer::recv::join_reply(const uint8_t* const data, size_t size, std::string& hash, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data;
    reader += sizeof(message::peer::Header);

    size_t hash_size = *(size_t*) reader;
    reader += sizeof(size_t);

    hash.resize(hash_size);
    memcpy(hash.data(), (char*) reader, hash_size);
    reader += hash_size;

    size_t remaining_size = size - sizeof(message::peer::Header) - sizeof(uint16_t) - sizeof(size_t) - hash_size;
    fragments_completed.resize(remaining_size);
    for (size_t x = 0; x < remaining_size; ++x) {
        fragments_completed[x] = *(bool*) reader;
        reader += sizeof(bool);
    }
    return true;
}

bool connections::peer::recv::leave(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port) {
    const uint8_t* reader = data;

    port = *(uint16_t*) reader;
    reader += sizeof(uint16_t);

    size_t hash_size = size - sizeof(message::peer::Header) - sizeof(uint16_t);

    hash.resize(hash_size);
    memcpy((char*) hash.data(), (char*) reader, hash_size);

    return true;
}

bool connections::peer::recv::data_req(const uint8_t* const data, size_t size, size_t& fragment_nr) {
    if (size != sizeof(message::peer::Header) + sizeof(size_t))
        return false;
    fragment_nr = *(size_t*) (data+sizeof(message::peer::Header));
    return true;
}

bool connections::peer::recv::data_reply(const uint8_t* const data, size_t size, size_t& fragment_nr, uint8_t*& fragment_data) {
    if (size <= sizeof(message::peer::Header) + sizeof(size_t))
        return false;
    fragment_nr = *(size_t*) (data+sizeof(message::peer::Header));
    fragment_data = (uint8_t*) (data+sizeof(message::peer::Header)+sizeof(size_t));
    return true;
}

// Message:
// Peer message header (tag=AVAILABILITY)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::recv::availability(const uint8_t* const data, size_t size, std::string& hash, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data+sizeof(message::peer::Header);
    
    // Size of hash
    size_t hash_size = *(size_t*) reader;
    reader += sizeof(size_t);

    // hash
    hash.resize(hash_size);
    memcpy(hash.data(), (char*) reader, hash_size);
    reader += hash_size;

    // fragment vector
    size_t remaining_size = size - sizeof(message::peer::Header) - sizeof(size_t) - hash_size;
    fragments_completed.resize(remaining_size);
    for (size_t x = 0; x < remaining_size; ++x) {
        fragments_completed[x] = *(bool*) reader;
        reader += sizeof(bool);
    }
    return true;
}

// Message:
// Peer message header (tag=AVAILABILITY)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::recv::availability_reply(const uint8_t* const data, size_t size, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data+sizeof(message::peer::Header);

    size_t remaining_size = size - sizeof(message::peer::Header);
    fragments_completed.resize(remaining_size);
    for (size_t x = 0; x < remaining_size; ++x) {
        fragments_completed[x] = *(bool*) reader;
        reader += sizeof(bool);
    }
    return true;
}