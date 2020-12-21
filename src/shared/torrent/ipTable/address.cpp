#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstring>


#include "address.h"

// Packs a ConnectionType into a byte
static uint8_t pack(ConnectionType type) {
    uint8_t MASK2 = (type.n_type.t == NetType::IPv4) ? 0b00000000 : 0b00000001;
    return 0b00000000 | MASK2;
}

// Unpacks a byte to a ConnectionType
static ConnectionType unpack(uint8_t byte) {
    NetType::Type t((byte == 0) ? NetType::IPv4 : NetType::IPv6); 
    return ConnectionType(TransportType(), NetType(t));
}

Address Address::from(std::istream& is) {
    Address a(ConnectionType(TransportType(), NetType()), "", 0);
    a.read_stream(is);
    return a;
}

Address Address::from_string(std::string& ip) {
    const unsigned nr_args = 4;
    std::vector<std::string> args(nr_args);
    std::stringstream addr_string(ip);
    std::string segment;
    for (unsigned i = 0; i < nr_args; ++i) {
        if (!std::getline(addr_string, segment, ':'))
            throw std::runtime_error("Trackerlist has wrong format");
        args[i] = segment;
    }

    if (args[0] != "TCP")
        throw std::runtime_error(args[0] + " is not a valid TransportType");
    TransportType::Type t = TransportType::TCP;

    int nettype;
    try {
        nettype = std::stoi(args[1]);
    } catch (const std::exception& e) {
        throw std::runtime_error("Could not convert '" + args[1] + "' to a NetType");
    }

    uint16_t port;
    try {
        port = (uint16_t)std::stoi(args[2]);
    } catch (const std::exception& e) {
        throw std::runtime_error("Could not convert '" + args[2] + "' to an integer");
    }

    if (nettype != 4 && nettype != 6)
        throw std::runtime_error(args[1] + " is not a valid NetType"); 

    NetType::Type n = (nettype == 4) ? NetType::IPv4 : NetType::IPv6;
    return Address(ConnectionType(t, n), args[3], port);
}

void Address::write_stream(std::ostream& os) const {
    os << pack(type);
    unsigned string_size = ip.size();
    os.write((char*)&string_size, sizeof(string_size));
    os.write((char*)ip.data(), string_size);
    os.write((char*)(&port), sizeof(port));
}

void Address::read_stream(std::istream& is) {
    uint8_t byte;
    is >> byte;
    type = unpack(byte);
    unsigned string_size; 
    is.read((char*)&string_size, sizeof(string_size));
    ip.resize(string_size);
    is.read((char*)ip.data(), string_size);
    is.read((char*)(&port), sizeof(port));
}

// Write address to buffer. Buffer contains:
// Connectiontype
// size (uint8_t)
// ip (string, ip_size() len)
// port (uint16_t)
uint8_t* Address::write_buffer(uint8_t* const buf) const {
    uint8_t* writer = buf;

    // Connectiontype
    *((ConnectionType*) writer) = type;
    writer += sizeof(ConnectionType);

    // IP size
    *writer = (uint8_t) ip.size(); // We can send size as uint8_t, as ip size is always lower than 255 (it is 16 or less)
    ++writer;

    // IP
    memcpy(writer, ip.data(), ip.size());
    writer += ip_size();

    // Port
    *((uint16_t*) writer) = port;
    writer += sizeof(uint16_t);
    return writer;
}

// Read address to buffer. Buffer contains:
// Connectiontype
// size (uint8_t)
// ip (string, ip_size() len)
// port (uint16_t)
const uint8_t* Address::read_buffer(const uint8_t* const buf) {
    const uint8_t* reader = buf;

    // Connectiontype
    type = *((ConnectionType*) reader);
    reader += sizeof(ConnectionType);

    // IP size
    size_t used_ip_size = (size_t) *reader;
    ++reader;

    // IP
    ip.resize(used_ip_size);
    memcpy(ip.data(), reader, used_ip_size);
    reader += ip_size();

    // Port
    port = *((uint16_t*) reader);
    reader += sizeof(uint16_t);
    return reader;
}