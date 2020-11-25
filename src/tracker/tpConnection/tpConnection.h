#ifndef TP_CONNECTION_TRACKER_H
#define TP_CONNECTION_TRACKER_H

#include "shared/connection/tracker/trackerConnection.h"
#include "shared/torrent/ipTable/ipTable.h"

// Peeks the type of request
TAG peek_request(std::unique_ptr<Connection>& connection);

// Retrieve the Peer and torrent-identifier
bool get_member(std::unique_ptr<Connection>& connection, std::string& hash, Addr& peer);
// Retrieve the torrent-identifier
bool get_table(std::unique_ptr<Connection>& connection, std::string& hash);
// Send the peertable
void send_table(std::unique_ptr<Connection>& connection, IPTable& peertable);
// Send only a header as response
void send_response(std::unique_ptr<Connection>& connection, TAG tag, STATUS status);

#endif