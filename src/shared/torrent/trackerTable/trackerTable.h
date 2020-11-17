#ifndef TRACKERTABLE_H
#define TRACKERTABLE_H

#include <cstdint>
#include <vector>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "shared/connection/meta/type.h"
#include "shared/torrent/file/streamable/streamable.h"
#include "trackerIP.h"



class TrackerTable : public Streamable {
public:

    TrackerTable(){};
    TrackerTable(std::vector<TrackerIP>& trackers) {this->trackers = trackers;};
    
    // Adds the IP address of a tracker to the table
    void add_tracker(ConnectionType sin_family, ConnectionType socket_type, std::string addr, uint16_t sin_port);
    
    // Adds the IP address of a tracker to the table
    void add_tracker(ConnectionType sin_family, ConnectionType socket_type, struct in_addr addr, uint16_t sin_port);
    
    // Adds the IP address of a tracker to the table, using default ConnectionTypes (ipv4, TCP)
    void add_tracker(std::string addr, uint16_t sin_port);
    
    // Adds the IP address of a tracker to the table, using default ConnectionTypes (ipv4, TCP)
    void add_tracker(struct in_addr addr, uint16_t sin_port);
    
    // Removes the IP address of the tracker from the table
    void remove_tracker(ConnectionType sin_family, std::string addr);
    
    // Removes the IP address of the tracker from the table
    void remove_tracker(struct in_addr addr);

    // Read and write to a SwarmTorrent file
    void write_stream(std::ostream& os) const override;
    
    void read_stream(std::istream& is) override;
private: 
    std::vector<TrackerIP> trackers;
};



#endif