#include <iostream>

#include "addr.h"

Addr Addr::from(std::istream& is) {
    Addr a("", 0); 
    a.read_stream(is);
    return a;
}

void Addr::write_stream(std::ostream& os) const {
    os.write((char*)(&ip), sizeof(ip));
    os.write((char*)(&port), sizeof(port));
}

void Addr::read_stream(std::istream& is) {
    is.read((char*)(&ip), sizeof(ip));
    is.read((char*)(&port), sizeof(port));
}