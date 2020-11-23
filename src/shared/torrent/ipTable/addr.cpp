#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>


#include "addr.h"

static uint8_t pack(ConnectionType type) {
    uint8_t MASK2 = (type.n_type.t == NetType::IPv4) ? 0b00000000 : 0b00000001;
    return 0b00000000 | MASK2;
}

static ConnectionType unpack(uint8_t byte) {
    NetType::Type t((byte == 0) ? NetType::IPv4 : NetType::IPv6); 
    return ConnectionType(TransportType(), NetType(t));
}

Addr Addr::from(std::istream& is) {
    Addr a(ConnectionType(TransportType(), NetType()), "", 0); 
    a.read_stream(is);
    return a;
}

Addr Addr::from_string(std::string ip) {
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
        port = (uint16_t)std::stoi(args[3]);
    } catch (const std::exception& e) {
        throw std::runtime_error("Could not convert '" + args[3] + "' to an integer");
    }

    if (nettype != 4 && nettype != 6)
        throw std::runtime_error(args[1] + " is not a valid NetType"); 

    NetType::Type n = (nettype == 4) ? NetType::IPv4 : NetType::IPv6;
    return Addr(ConnectionType(t, n), args[2], port);
}

void Addr::write_stream(std::ostream& os) const {
    os << pack(type);
    os.write((char*)(&ip), sizeof(ip));
    os.write((char*)(&port), sizeof(port));
}

void Addr::read_stream(std::istream& is) {
    uint8_t byte;
    is >> byte;
    type = unpack(byte);
    is.read((char*)(&ip), sizeof(ip));
    is.read((char*)(&port), sizeof(port));
}