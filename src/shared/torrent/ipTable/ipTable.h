#ifndef TRACKERTABLE_H
#define TRACKERTABLE_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>

#include "shared/torrent/file/streamable/streamable.h"
#include "addr.h"


class IPTable : public Streamable {
public:
    IPTable(){};
    IPTable(std::unordered_map<std::string, Addr>& ips) : ips(ips) {};
    
    // Constructs an IPTable using a vector of strings
    // Strings should have the following format: 
    // TransportType:NetType:PORT:IP
    static IPTable from(std::vector<std::string>& ips);

    // Adds the Addr to the table if not already there
    // Returns whether insertion is success
    bool add_ip(Addr a) { return ips.insert({a.ip, Addr(a.type, a.ip, a.port)}).second; };

    // Adds an address to the table with its IP and port
    // Returns whether insersion is success
    bool add_ip(ConnectionType type, std::string ip, uint16_t port) { return ips.insert({ip, Addr(type, ip, port)}).second; };

    // Removes Addr a from the table
    void remove_ip(Addr a) { ips.erase(a.ip); };

    // Removes Addr via its ip 
    void remove_ip(std::string ip) { ips.erase(ip); };

    // Sets Addr struct of ip string, if it exists
    // Returns whether ip string exists in table
    bool get_Addr(std::string ip, Addr& a) const;

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    
    void read_stream(std::istream& is) override;
private: 
    std::unordered_map<std::string, Addr> ips;
};



#endif