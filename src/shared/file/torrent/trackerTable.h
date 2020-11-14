#ifndef TRACKERTABLE_H
#define TRACKERTABLE_H

#include <vector>
#include <string>
#include <utility>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../connection/meta/type.h"
#include "tracker_ip.h"


class TrackerTable {
public:
    TrackerTable(){};
    TrackerTable(std::vector<Tracker_IP>& trackers) {this->trackers = trackers;};

    //adds the IP address of a tracker to the table
    void add_tracker(ConnectionType sin_family, ConnectionType socket_type, std::string tracker_ip, unsigned short sin_port);
    //adds the IP address of a tracker to the table
    void add_tracker(ConnectionType sin_family, ConnectionType socket_type, struct in_addr tracker_ip, unsigned short sin_port);
    //adds the IP address of a tracker to the table, using default ConnectionTypes (ipv4, TCP)
    void add_tracker(std::string tracker_ip, unsigned short sin_port);
    //adds the IP address of a tracker to the table, using default ConnectionTypes (ipv4, TCP)
    void add_tracker(struct in_addr tracker_ip, unsigned short sin_port);
    //removes the IP address of the tracker from the table
    void remove_tracker(std::string tracker_ip);
    //removes the IP address of the tracker from the table
    void remove_tracker(struct in_addr tracker_ip);

    //read and write the TrackerTable class in byte format
    friend std::ostream& operator<<(std::ostream& os, const TrackerTable& trackerTable);
    friend std::istream& operator>>(std::istream& is, TrackerTable& trackerTable);
private: 
    std::vector<Tracker_IP> trackers;

};



#endif