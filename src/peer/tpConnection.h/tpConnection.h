#ifndef TP_CONNECTION_PEER_H
#define TP_CONNECTION_PEER_H

#include <string>
#include <memory>

#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/torrent/ipTable/ipTable.h"

// Subscribe peer to peertable of tracker
bool subscribe(std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer);
// Unsubscribe peer to peertable of tracker
bool unsubscribe(std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer);
// Retrieve peertable from tracker
bool retrieve(std::unique_ptr<Connection>& connection, std::string torrent_hash, IPTable& peertable);
// Update peertable to tracker
// TODO:
//bool update(std::unique_ptr<Connection> connection, std::string torrent_hash, ); 

#endif