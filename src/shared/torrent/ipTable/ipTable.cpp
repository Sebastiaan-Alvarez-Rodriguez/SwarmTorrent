#include "ipTable.h"


IPTable IPTable::from(std::vector<std::string>& ips) {
    IPTable ip_table;
    for (std::string ip : ips)
        ip_table.add(Address::from_string(ip));
    return ip_table;
}

bool IPTable::get_addr(std::string ip, uint16_t port, Address& a) const {
    a.ip = ip;
    a.port = port;
    auto item = ips.find(a);
    if (item == ips.end())
        return false;
    a = *item;
    return true;
}

void IPTable::write_stream(std::ostream& os) const {
    unsigned size = ips.size();
    os.write((char*)(&size), sizeof(size));
    for (auto ip : ips) 
        ip.write_stream(os);
}

void IPTable::read_stream(std::istream& is) {
    unsigned size; 
    is.read((char*)(&size), sizeof(size));
    for (unsigned i = 0; i < size; ++i) 
        add(Address::from(is));
}  