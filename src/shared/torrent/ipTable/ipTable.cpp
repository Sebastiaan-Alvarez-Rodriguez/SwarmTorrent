#include <sstream>
#include <stdexcept>

#include "ipTable.h"

static Addr parse_string(std::string ip) {
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

    int nettype = std::stoi(args[1]);
    if (nettype != 4 && nettype != 6)
        throw std::runtime_error(nettype + " is not a valid NetType"); 
    NetType::Type n = (nettype == 4) ? NetType::IPv4 : NetType::IPv6;
    return Addr(ConnectionType(t, n), args[2], (uint16_t)std::stoi(args[3]));
}

IPTable IPTable::from(std::vector<std::string> ips) {
    IPTable ip_table;
    for (std::string ip : ips)
        ip_table.add_ip(parse_string(ip));
    return ip_table;
}

bool IPTable::get_Addr(std::string ip, Addr& a) const {
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
        add_ip(Addr::from(is));
}  