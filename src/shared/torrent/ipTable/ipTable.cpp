#include "ipTable.h"

bool IPTable::get_Addr(std::string ip, Addr& a) const {
    auto item = ips.find(ip);
    if (item == ips.end())
        return false;
    auto addr = *item;
    a = Addr(addr.first, addr.second);
    return true;
}

void IPTable::write_stream(std::ostream& os) const {
    unsigned size = ips.size();
    os.write((char*)(&size), sizeof(size));
    for (auto ip : ips) 
        Addr(ip.first, ip.second).write_stream(os);
}

void IPTable::read_stream(std::istream& is) {
    unsigned size; 
    is.read((char*)(&size), sizeof(size));
    for (unsigned i = 0; i < size; ++i) 
        add_ip(Addr::from(is));
}  