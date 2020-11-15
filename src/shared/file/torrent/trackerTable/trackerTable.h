#ifndef TRACKERTABLE_H
#define TRACKERTABLE_H

#include <vector>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../../connection/meta/type.h"
#include "../swarmTorrentWriter.h"
#include "tracker_ip.h"



class TrackerTable : public SwarmTorrentWriter {
public:
    TrackerTable(){};
    TrackerTable(std::vector<Tracker_IP>& trackers) {this->trackers = trackers;};

    //Adds the IP address of a tracker to the table
    void add_tracker(ConnectionType sin_family, ConnectionType socket_type, std::string tracker_ip, unsigned short sin_port);
    //Adds the IP address of a tracker to the table
    void add_tracker(ConnectionType sin_family, ConnectionType socket_type, struct in_addr tracker_ip, unsigned short sin_port);
    //Adds the IP address of a tracker to the table, using default ConnectionTypes (ipv4, TCP)
    void add_tracker(std::string tracker_ip, unsigned short sin_port);
    //Adds the IP address of a tracker to the table, using default ConnectionTypes (ipv4, TCP)
    void add_tracker(struct in_addr tracker_ip, unsigned short sin_port);
    //Removes the IP address of the tracker from the table
    void remove_tracker(std::string tracker_ip);
    //Removes the IP address of the tracker from the table
    void remove_tracker(struct in_addr tracker_ip);

    //Read and write to a SwarmTorrent file
    void write_swarm(std::ostream& os) const override;
    void read_swarm(std::istream& is) override;
private: 
    std::vector<Tracker_IP> trackers;

};



#endif