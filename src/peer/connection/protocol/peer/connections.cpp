#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include "peer/connection/message/peer/message.h"
#include "shared/connection/message/message.h"
#include "shared/torrent/file/io/fragmentHandler.h"
#include "connections.h"


/**
 * Prepares a request, by allocating a buffer for given datasize, 
 * starting with a message::peer::Header of given type.
 * 
 * '''Note:''' size of a message::peer::Header is automatically appended to the datasize. Also: caller has to free returned buffer.
 * @return pointer to start of buffer.
 */
static inline uint8_t* prepare_peer_message(size_t datasize, message::peer::Tag tag) {
    uint8_t* const data = (uint8_t*) malloc(message::peer::bytesize()+datasize);
    message::peer::from(datasize, tag).write(data);
    return data;
}

// Same as above function, but then for standard messages
static inline uint8_t* prepare_standard_message(size_t datasize, message::standard::Tag tag) {
    uint8_t* const data = (uint8_t*) malloc(message::standard::bytesize()+datasize);
    message::standard::from(datasize, tag).write(data);
    return data;
}

bool connections::peer::test(__attribute__ ((unused)) std::unique_ptr<ClientConnection>& connection) {
    std::cerr << "P2P testing function ready for implementation\n";
    return false;
}

// Message:
// Peer message header (tag=JOIN)
// port (uint16_t)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::send::join(std::unique_ptr<ClientConnection>& connection, uint16_t port, const std::string& torrent_hash, const std::vector<bool>& fragments_completed) {
    const auto datasize = sizeof(uint16_t)+sizeof(size_t)+torrent_hash.size()+fragments_completed.size();
    uint8_t* const data = prepare_peer_message(datasize, message::peer::JOIN);
    uint8_t* writer = data + message::peer::bytesize();

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

    bool val = connection->sendmsg(data, message::peer::bytesize()+datasize);
    free(data);
    return val;
}

//message:
// standard header (tag=OK)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::send::join_reply(const std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, const std::vector<bool>& fragments_completed) {
    // vector of bools is internally packed. However, getting the data like that is impossible in a clean way.
    // Have to use byte-wise sending instead of packed
    const auto datasize = sizeof(size_t) + torrent_hash.size() + fragments_completed.size();
    uint8_t* const data = (uint8_t*) prepare_standard_message(datasize, message::standard::OK);
    uint8_t* writer = data + message::standard::bytesize();

    *(size_t*) writer = torrent_hash.size();
    writer += sizeof(size_t);

    memcpy(writer, torrent_hash.c_str(), torrent_hash.size());
    writer += torrent_hash.size();

    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    if (!connection->sendmsg(data, message::standard::bytesize()+datasize)) {
        std::cerr << "Could not reply to peer "; connection->print(std::cerr); std::cerr << '\n';
        free(data);
        return false;
    }
    free(data);
    return true;
}

bool connections::peer::send::leave(std::unique_ptr<ClientConnection>& connection, const std::string& torrent_hash, uint16_t port) {
    auto datasize = sizeof(uint16_t)+torrent_hash.size();
    uint8_t* const data = prepare_peer_message(datasize, message::peer::LEAVE);
    uint8_t* writer = data + message::peer::bytesize();

    *(uint16_t*) writer = port;
    writer += sizeof(uint16_t);

    memcpy(writer, torrent_hash.c_str(), datasize);

    bool val = connection->sendmsg(data, message::peer::bytesize()+datasize);
    free(data);
    return val;
}

// Sends a data request to other peer. Message contains:
// peer header (tag DATA_REQ)
// port (uint16_t)
// fragment nr (size_t)
bool connections::peer::send::data_req(std::unique_ptr<ClientConnection>& connection, uint16_t port, size_t fragment_nr) {
    const auto datasize = sizeof(uint16_t) + sizeof(size_t);
    uint8_t* const data = prepare_peer_message(datasize, message::peer::DATA_REQ);
    uint8_t* writer = data + message::peer::bytesize();

    *((uint16_t*) writer) = port;
    writer += sizeof(uint16_t);

    *((size_t*) writer) = fragment_nr;
    bool val = connection->sendmsg(data, message::peer::bytesize()+datasize);
    free(data);
    return val;
}

bool connections::peer::send::data_rej(std::unique_ptr<ClientConnection> &connection, const std::vector<bool>& fragments_completed) {
    const auto datasize = fragments_completed.size();
    uint8_t* const data = (uint8_t*) prepare_standard_message(datasize, message::standard::REJECT);
    uint8_t* writer = data + message::standard::bytesize();

    // vector of bools is internally packed. However, getting the data like that is impossible in a clean way.
    // Have to use byte-wise sending instead of packed
    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    bool val = connection->sendmsg(data, message::standard::bytesize()+datasize);
    free(data);
    return val;
}

bool connections::peer::send::data_reply_fast(const std::unique_ptr<ClientConnection>& connection, size_t fragment_nr, uint8_t* data, unsigned size) {
    message::peer::from_r(size, message::peer::DATA_REPLY).write(data);
    *((size_t*) (data+message::peer::bytesize())) = fragment_nr;
    bool val = connection->sendmsg(data, size);
    free(data);
    return val;
}

bool connections::peer::send::inquire(const std::unique_ptr<ClientConnection>& connection) {
    uint8_t* const data = prepare_peer_message(0, message::peer::INQUIRE);
    return connection->sendmsg(data, message::peer::bytesize());
}

// Message:
// Peer message header (tag=AVAILABILITY)
// registered port (uint16_t)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::send::availability(const std::unique_ptr<ClientConnection>& connection, uint16_t port, const std::string& hash, const std::vector<bool>& fragments_completed) {
    const size_t datasize = sizeof(uint16_t) + sizeof(size_t) + hash.size() + fragments_completed.size();
    uint8_t* const buf = prepare_peer_message(datasize, message::peer::AVAILABILITY);
    uint8_t* writer = buf + message::standard::bytesize();

    // registered port
    *(uint16_t*) writer = port;
    writer += sizeof(uint16_t);

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

    if (!connection->sendmsg(buf, message::peer::bytesize()+datasize)) {
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
    const size_t datasize = fragments_completed.size();
    uint8_t* const buf = prepare_standard_message(datasize, message::standard::OK);
    uint8_t* writer = buf+ message::standard::bytesize();

    for (bool b : fragments_completed) {
        *(bool*) writer = b;
        writer += sizeof(bool);
    }

    if (!connection->sendmsg(buf, message::standard::bytesize()+datasize)) {
        std::cerr << "Could not reply to peer "; connection->print(std::cerr); std::cerr << '\n';
        free(buf);
        return false;
    }
    free(buf);
    return true;
}





// Message:
// Peer message header (tag=JOIN)
// port (uint16_t)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::recv::join(const uint8_t* const data, size_t size, std::string& hash, uint16_t& port, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data;
    reader += message::peer::bytesize();

    port = *(uint16_t*) reader;
    reader += sizeof(uint16_t);

    size_t hash_size = *(size_t*) reader;
    reader += sizeof(size_t);

    hash.resize(hash_size);
    memcpy((char*) hash.data(), (char*) reader, hash_size);
    reader += hash_size;

    const size_t remaining_size = size - message::peer::bytesize() - sizeof(uint16_t) - sizeof(size_t) - hash_size;
    fragments_completed.resize(remaining_size);
    for (size_t x = 0; x < remaining_size; ++x) {
        fragments_completed[x] = *(bool*) reader;
        reader += sizeof(bool);
    }

    return true;
}

//message:
// standard header (tag=OK)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::recv::join_reply(const uint8_t* const data, size_t size, std::string& hash, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data + message::standard::bytesize();

    size_t hash_size = *(size_t*) reader;
    reader += sizeof(size_t);

    hash.resize(hash_size);
    memcpy(hash.data(), (char*) reader, hash_size);
    reader += hash_size;

    const size_t remaining_size = size - message::standard::bytesize() - sizeof(size_t) - hash_size;
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

    size_t hash_size = size - message::peer::bytesize() - sizeof(uint16_t);

    hash.resize(hash_size);
    memcpy((char*) hash.data(), (char*) reader, hash_size);

    return true;
}

// Sends a data request to other peer. Message contains:
// peer header (tag DATA_REQ)
// port (uint16_t)
// fragment nr (size_t)
bool connections::peer::recv::data_req(const uint8_t* const data, __attribute__ ((unused)) size_t size, uint16_t& port, size_t& fragment_nr) {
    const uint8_t* reader = data+message::peer::bytesize();

    port = *(uint16_t*) reader;
    reader += sizeof(uint16_t);

    fragment_nr = *(size_t*) reader;
    return true;
}

bool connections::peer::recv::data_reply(const uint8_t* const data, size_t size, size_t& fragment_nr, uint8_t*& fragment_data) {
    if (size <= message::peer::bytesize() + sizeof(size_t))
        return false;
    fragment_nr = *(size_t*) (data+message::peer::bytesize());
    fragment_data = (uint8_t*) (data+message::peer::bytesize()+sizeof(size_t));
    return true;
}

// Message:
// Peer message header (tag=AVAILABILITY)
// registered port (uint16_t)
// size of hash (size_t)
// hash (string)
// fragment vector (raw bool array, byte-aligned)
bool connections::peer::recv::availability(const uint8_t* const data, size_t size, uint16_t& port, std::string& hash, std::vector<bool>& fragments_completed) {
    const uint8_t* reader = data+message::peer::bytesize();

    // registered port
    port = *(uint16_t*) reader;
    reader += sizeof(uint16_t);

    
    // Size of hash
    size_t hash_size = *(size_t*) reader;
    reader += sizeof(size_t);

    // hash
    hash.resize(hash_size);
    memcpy(hash.data(), (char*) reader, hash_size);
    reader += hash_size;

    // fragment vector
    size_t remaining_size = size - message::peer::bytesize() - sizeof(size_t) - hash_size;
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
    const uint8_t* reader = data+message::peer::bytesize();

    size_t remaining_size = size - message::peer::bytesize();
    fragments_completed.resize(remaining_size);
    for (size_t x = 0; x < remaining_size; ++x) {
        fragments_completed[x] = *(bool*) reader;
        reader += sizeof(bool);
    }
    return true;
}