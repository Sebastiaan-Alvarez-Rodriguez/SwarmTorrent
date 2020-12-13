#include <cstdint>
#include <cstring>
#include <iostream>

#include "shared/connection/connection.h"
#include "shared/torrent/ipTable/ipTable.h"

#include "connections.h"

bool connections::shared::send::peertable(const std::unique_ptr<ClientConnection>& connection, const IPTable& table, const std::string& hash, message::standard::Tag tag) {
    size_t table_size = sizeof(size_t) + hash.size() + table.size() * Address::size();
    uint8_t* const table_buffer = (uint8_t*) malloc(sizeof(message::standard::Header)+table_size);
    uint8_t* writer = table_buffer;
    *((message::standard::Header*) writer) = message::standard::from(table_size, tag);
    writer += sizeof(message::standard::Header);

    *(size_t*) writer = (size_t) hash.size();
    writer += sizeof(size_t);

    memcpy(writer, hash.data(), hash.size());
    writer += hash.size();

    for (auto it = table.cbegin(); it != table.cend(); ++it)
        writer = it->second.write_buffer(writer);

    if (!connection->sendmsg(table_buffer, sizeof(message::standard::Header)+table_size)) {
        std::cerr << "Problem sending peertable to: "; connection->print(std::cerr); std::cerr << '\n';
        free(table_buffer);
        return false;
    }
    std::cerr << "Sent table containing " << table.size() << " entries:\n";
    for (auto it = table.cbegin(); it != table.cend(); ++it) {
        std::cerr << it->second.ip << ':' << it->second.port << '\n';
    }
    free(table_buffer);
    return true;
}

bool connections::shared::recv::peertable(const uint8_t* const data, size_t size, IPTable& peertable, std::string& hash) {
    const uint8_t* ptr = data+sizeof(message::standard::Header);
    const size_t hash_size = *(size_t*) (ptr);
    hash.resize(hash_size);
    ptr += sizeof(size_t);

    memcpy(hash.data(), ptr, hash_size);
    ptr += hash_size;

    // Rest of message body contains a number of addresses, together forming the table
    // Each address is const-size, so we can get amount of addresses simply by doing below.
    const size_t amount = (size - sizeof(message::standard::Header) - sizeof(size_t) - hash_size) / Address::size();
    for (size_t x = 0; x < amount; ++x) {
        Address a;
        ptr = a.read_buffer(ptr);
        if (!peertable.add_ip(a))
            return false;
    }
    return true;
}