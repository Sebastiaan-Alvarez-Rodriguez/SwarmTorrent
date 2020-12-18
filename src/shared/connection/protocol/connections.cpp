#include <cstdint>
#include <cstring>
#include <iostream>

#include "shared/connection/connection.h"
#include "shared/torrent/ipTable/ipTable.h"

#include "connections.h"

// Same as above function, but then for standard messages
static inline uint8_t* prepare_standard_message(size_t datasize, message::standard::Tag tag) {
    uint8_t* const data = (uint8_t*) malloc(message::standard::bytesize()+datasize);
    message::standard::from(datasize, tag).write(data);
    return data;
}

// Sends a LOCAL_DISCOVERY_REQ to remote.
// hash (string)
bool connections::shared::send::discovery_req(const std::unique_ptr<ClientConnection>& connection, const std::string& hash) {
    const auto m_size = hash.size();
    uint8_t* const data = prepare_standard_message(m_size, message::standard::LOCAL_DISCOVERY_REQ);
    uint8_t* writer = data + message::standard::bytesize();

    memcpy(writer, hash.c_str(), m_size);
    bool val = connection->sendmsg(data, message::standard::bytesize()+m_size);
    free(data);
    return val;
}

// Sends a LOCAL_DISCOVERY_REPLY to remote.
// hash_size (size_t)
// hash (string)
// table (array of const-sized Address)
bool connections::shared::send::discovery_reply(const std::unique_ptr<ClientConnection>& connection, const IPTable& table, const std::string& hash, const Address& addr) {
    size_t table_size = sizeof(size_t) + hash.size() + (1+table.size()) * Address::size();
    uint8_t* const data = prepare_standard_message(table_size, message::standard::LOCAL_DISCOVERY_REPLY);
    uint8_t* writer = data + message::standard::bytesize();

    // Write size of hash
    *(size_t*) writer = (size_t) hash.size();
    writer += sizeof(size_t);

    // Write hash
    memcpy(writer, hash.data(), hash.size());
    writer += hash.size();

    // Write own address
    writer = addr.write_buffer(writer);

    // Write table
    for (auto it = table.cbegin(); it != table.cend(); ++it)
        writer = it->write_buffer(writer);

    if (!connection->sendmsg(data, message::standard::bytesize()+table_size)) {
        std::cerr << "Problem sending peertable to: "; connection->print(std::cerr); std::cerr << '\n';
        free(data);
        return false;
    }
    std::cerr << "Sent table containing " << table.size() << " entries:\n";
    for (auto it = table.cbegin(); it != table.cend(); ++it) {
        std::cerr << it->ip << ':' << it->port << '\n';
    }
    free(data);
    return true;
}

// Receive information of a LOCAL_DISCOVERY_REQ from a raw buffer
// hash (string)
bool connections::shared::recv::discovery_req(const uint8_t* const data, size_t size, std::string& hash) {
    hash = std::string((char*)(data+message::standard::bytesize()), size-message::standard::bytesize());
    return true;
}

// Receive information of a LOCAL_DISCOVERY_REPLY from a raw buffer
// hash_size (size_t)
// hash (string)
// table (array of const-sized Address)
bool connections::shared::recv::discovery_reply(const uint8_t* const data, size_t size, IPTable& peertable, std::string& hash) {
    const uint8_t* ptr = data+message::standard::bytesize();

    // Read hash size    
    const size_t hash_size = *(size_t*) ptr;
    ptr += sizeof(size_t);

    // Read hash
    hash.resize(hash_size);
    memcpy(hash.data(), ptr, hash_size);
    ptr += hash_size;

    // Rest of message body contains a number of addresses, together forming the table
    // Each address is const-size, so we can get amount of addresses simply by doing below.
    const size_t amount = (size - message::standard::bytesize() - sizeof(size_t) - hash_size) / Address::size();
    for (size_t x = 0; x < amount; ++x) {
        Address a;
        ptr = a.read_buffer(ptr);
        if (!peertable.add(a))
            return false;
    }
    return true;
}