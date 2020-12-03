#ifndef TRACKERTABLE_H
#define TRACKERTABLE_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>

#include "shared/torrent/file/streamable/streamable.h"
#include "address.h"


class IPTable : public Streamable {
public:
    IPTable() = default;
    explicit IPTable(std::unordered_map<std::string, Address>& ips) : ips(ips) {};
    
    // Constructs an IPTable using a vector of strings
    // Strings should have the following format: 
    // TransportType:NetType:PORT:IP
    static IPTable from(std::vector<std::string>& ips);

    // Adds the Address to the table if not already there
    // Returns whether insertion is success
    bool add_ip(const Address& a) { return ips.insert({a.ip, Address(a.type, a.ip, a.port)}).second; };

    // Adds an address to the table with its IP and port
    // Returns whether insersion is success
    bool add_ip(ConnectionType type, const std::string& ip, uint16_t port) { return ips.insert({ip, Address(type, ip, port)}).second; };

    // Removes Address a from the table
    //TODO @Mariska: Erasure on hashmaps is expensive if it triggers recomputation of the hash. Maybe just use 'dirty' flag?
    void remove_ip(const Address& a) { ips.erase(a.ip); };

    // Removes Address via its ip
    //TODO @Mariska: Erasure on hashmaps is expensive if it triggers recomputation of the hash. Maybe just use 'dirty' flag?
    void remove_ip(const std::string& ip) { ips.erase(ip); };

    // Sets Address struct of ip string, if it exists
    // Returns whether ip string exists in table
    //TODO @Mariska: Comments in 2 lines above this make no sense?
    bool get_Addr(std::string ip, Address& a) const;

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    
    void read_stream(std::istream& is) override;

    inline auto iterator_begin() const { return ips.begin(); }
    inline auto iterator_end() const { return ips.end(); }

    inline void merge(const IPTable& other) {
        ips.insert(other.iterator_begin(), other.iterator_end());
    }
    inline size_t size() const { return ips.size(); }
protected: 
    std::unordered_map<std::string, Address> ips;
};



#endif