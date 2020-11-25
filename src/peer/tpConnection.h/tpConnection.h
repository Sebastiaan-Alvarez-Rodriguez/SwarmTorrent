#ifndef TP_CONNECTION_PEER_H
#define TP_CONNECTION_PEER_H

#include <string>
#include <memory>

#include "shared/connection/impl/TCP/TCPConnection.h"
#include "shared/torrent/ipTable/ipTable.h"

bool subscribe(std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer);
bool unsubscribe(std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer);
bool retrieve(std::unique_ptr<Connection>& connection, std::string torrent_hash, IPTable& peertable);
//bool update(std::unique_ptr<Connection> connection, std::string torrent_hash, ); 

#endif