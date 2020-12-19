#ifndef TRACKERTABLE_H
#define TRACKERTABLE_H

#include <iostream>
#include <cstdint>
#include <unordered_set>
#include <string>
#include <vector>

#include "shared/torrent/file/streamable/streamable.h"
#include "address.h"


class IPTable : public Streamable {
public:
    IPTable() = default;

    explicit IPTable(std::unordered_set<Address>&& ips) : ips(std::move(ips)) {};
    
    // Constructs an IPTable using a vector of strings
    // Strings should have the following format: 
    // TransportType:NetType:PORT:IP
    static IPTable from(std::vector<std::string>& ips);

    // Adds the Address to the table if not already there
    // Returns whether insertion is success
    bool add(const Address& a) { return ips.insert(a).second; };
    bool add(const Address&& a) { return ips.insert(std::move(a)).second; }

    // Removes Address a from the table
    void remove(const Address& a) { ips.erase(a); };

    // If IPTable contains ip string, sets Address struct corresponding to ip string.
    // Returns whether IPTable contains ip string
    bool get_addr(std::string ip, uint16_t port, Address& a) const;

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    
    void read_stream(std::istream& is) override;

    inline bool contains(const Address& a) const { return ips.find(a) != ips.end(); }
    inline auto cbegin() const { return ips.begin(); }
    inline auto cend() const { return ips.end(); }

    inline void merge(const IPTable& other) {
        ips.insert(other.cbegin(), other.cend());
    }
    inline size_t size() const { return ips.size(); }

    inline void print(std::ostream& stream) {
        stream << "IPTable: \n";
        for (auto it = cbegin(); it != cend(); ++it)
            stream << "\tentry: " << it->type << ", " << it->ip << ':' << it->port << '\n';
    }

    inline IPTable copy() const {
        return IPTable(*this);
    }
protected:
    // a set of addresses
    std::unordered_set<Address> ips;
};



#endif