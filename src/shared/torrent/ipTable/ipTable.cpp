#include "ipTable.h"


IPTable IPTable::from(std::vector<std::string>& ips) {
    IPTable ip_table;
    for (std::string ip : ips)
        ip_table.add_ip(Address::from_string(ip));
    return ip_table;
}

bool IPTable::get_addr(std::string ip, Address& a) const {
    auto item = ips.find(ip);
    if (item == ips.end())
        return false;
    a = (*item).second;
    return true;
}

void IPTable::write_stream(std::ostream& os) const {
    unsigned size = ips.size();
    os.write((char*)(&size), sizeof(size));
    for (auto ip : ips) 
        ip.second.write_stream(os);
}

void IPTable::read_stream(std::istream& is) {
    unsigned size; 
    is.read((char*)(&size), sizeof(size));
    for (unsigned i = 0; i < size; ++i) 
        add_ip(Address::from(is));
}  