#include <cstdint>
#include <cstring>
#include <iostream>

#include "connections.h"

bool connections::shared::send::peertable(const std::unique_ptr<ClientConnection>& connection, const IPTable& peertable, const std::string& hash, message::standard::Tag tag) {
    size_t table_size = hash.size() + table.size() * Address::size();
    uint8_t* const table_buffer = (uint8_t*) malloc(sizeof(message::standard::Header)+table_size);
    uint8_t* writer = table_buffer;
    *((message::standard::Header*) writer) = message::standard::from(table_size, message::standard::OK);
    writer += sizeof(message::standard::Header);

    memcpy(writer, hash.data(), hash.size());
    writer += hash.size();

    for (auto it = table.cbegin(); it != table.cend(); ++it)
        writer = it->second.write_buffer(writer);

    if (!client_conn->sendmsg(table_buffer, sizeof(message::standard::Header)+table_size)) {
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

bool connections::shared::recv::peertable(const std::unique_ptr<ClientConnection>& connection, const IPTable& peertable, message::standard::Tag tag) {
    message::standard::Header header;
    connection->peekmsg((uint8_t*)&header, sizeof(header));

    uint8_t* const table_buffer = (uint8_t*)malloc(header.size);
    connection->recvmsg(table_buffer, header.size);

    // Body of the message only contains a number of addresses.
    // Each address is const-size, so we can get amount of addresses simply by doing below.
    const size_t amount = (header.size - sizeof(header)) / Address::size();
    const uint8_t* reader = table_buffer + sizeof(header);
    for (size_t x = 0; x < amount; ++x) {
        Address a;
        reader = a.read_buffer(reader);
        if (!peertable.add_ip(a))
            return false;
    }
    free(table_buffer);
    return true;
}