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

    // Adds an address to the table with its IP and sourcePort
    // Returns whether insersion is success
    bool add_ip(ConnectionType type, const std::string& ip, uint16_t port) { return ips.insert({ip, Address(type, ip, port)}).second; };

    // Removes Address a from the table
    void remove_ip(const Address& a) { ips.erase(a.ip); };

    // Removes Address via its ip
    void remove_ip(const std::string& ip) { ips.erase(ip); };

    // If IPTable contains ip string, sets Address struct corresponding to ip string.
    // Returns whether IPTable contains ip string
    bool get_addr(std::string ip, Address& a) const;

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    
    void read_stream(std::istream& is) override;

    inline bool contains(const std::string& ip) const { return ips.find(ip) != ips.end(); }
    inline auto cbegin() const { return ips.begin(); }
    inline auto cend() const { return ips.end(); }

    inline void merge(const IPTable& other) {
        ips.insert(other.cbegin(), other.cend());
    }
    inline size_t size() const { return ips.size(); }

    inline void print(std::ostream& stream) {
        stream << "IPTable: \n";
        for (auto it = cbegin(); it != cend(); ++it)
            stream << "\tentry: " << it->second.type << ", " << it->second.ip << ':' << it->second.port << '\n';
    }
protected: 
    std::unordered_map<std::string, Address> ips;
};



#endif