#include <iostream>

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