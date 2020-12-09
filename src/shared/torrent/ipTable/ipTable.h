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
    //TODO @Mariska: Erasure on hashmaps is expensive if it triggers recomputation of the hash. Maybe just use 'dirty' flag?
    void remove_ip(const Address& a) { ips.erase(a.ip); };

    // Removes Address via its ip
    //TODO @Mariska: Erasure on hashmaps is expensive if it triggers recomputation of the hash. Maybe just use 'dirty' flag?
    void remove_ip(const std::string& ip) { ips.erase(ip); };

    // If IPTable contains ip string,
    // Sets Address struct corresponding to ip string
    // Returns whether IPTable contains ip string
    //TODO @Mariska: Comments in 2 lines above this make no sense?
    //TODO @Sebastiaan: better?
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

    inline void send_table_stub() {
        // Contains some info on how to intelligently load a peertable onto a buffer
        // auto peertable_size = sizeof(size_t) + peertable.size() * Address::size();
        // *((size_t*) writer) = peertable.size();
        // writer += sizeof(size_t);

        // for (auto it = peertable.iterator_begin(); it != peertable.iterator_end(); ++it) 
        //     writer = (*it).second.write_buffer(writer);
    }

    inline void recv_table_stub() {
        // TODO: use reserve
        // TODO: find fast insert 
        // Contains some info on how to intellibently load a peertable from a buffer
        // Note: @Sebastiaan does not agree with some of the content here: 
        // "Please don't send an error message when insertion fails!"
        // size_t nr_peers = *((size_t*) reader);
        // reader += sizeof(size_t);

        // IPTable table;
        // for (size_t i = 0; i < nr_peers; ++i) {
        //     Address a(ConnectionType(TransportType(), NetType()), "", 0);
        //     reader = a.read_buffer(reader);
        //     if (!table.add_ip(a)) {
        //         message::standard::send(client_conn, message::standard::ERROR);
        //         return;
        //     }
        // }
    }
protected: 
    std::unordered_map<std::string, Address> ips;
};



#endif