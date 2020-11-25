#include <sstream>
#include <cstring>

#include "tpConnection.h"
#include "shared/connection/tracker/trackerConnection.h"

static void request(std::unique_ptr<Connection>& connection, Header header, size_t msg_length, const void* const msg) {
    void* const buf = malloc(sizeof(Header) + msg_length);
    memcpy(buf, &header, sizeof(Header));
    memcpy((uint8_t*)buf + sizeof(Header), msg, msg_length);

    connection->sendmsg(buf, sizeof(Header)+msg_length);
    free(buf);
}

static STATUS receive(std::unique_ptr<Connection>& connection, size_t& msg_length, void* msg) {
    Header header;
    connection->recvmsg(&header, sizeof(Header));

    if (header.status == ERROR)
        return ERROR; 

    msg_length = header.size;
    msg = malloc(msg_length);
    connection->recvmsg(msg, msg_length);

    return header.status;
}

static bool update_member(bool subscribe, std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer) {
    std::stringstream ss; 
    //serialize torrent_hash and peer
    ss << torrent_hash.size();
    ss.write((char*)torrent_hash.data(), torrent_hash.size());
    peer.write_stream(ss);

    //prepare to request
    std::string tmp = ss.str();
    size_t msg_length = tmp.size(); 
    Header header = {(subscribe) ? SUBSCRIBE : UNSUBSCRIBE, REQUEST, msg_length};
    const char* msg = tmp.c_str();
    request(connection, header, msg_length, msg);

    char* received = nullptr;
    if (receive(connection, msg_length, received) == ERROR)
        return false;

    free(received);
    return true; 
}

bool subscribe(std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer) {    
    return update_member(true, connection, torrent_hash, peer);
}

bool unsubscribe(std::unique_ptr<Connection>& connection, std::string torrent_hash, Addr peer) {
    return update_member(false, connection, torrent_hash, peer);
}

bool retrieve(std::unique_ptr<Connection>& connection, std::string torrent_hash, IPTable& peertable) {
    size_t msg_length = torrent_hash.size();
    Header header = {RECEIVE, REQUEST, msg_length};
    const char* const buf = torrent_hash.c_str();
    request(connection, header, msg_length, buf);

    char* received = nullptr;
    if (receive(connection, msg_length, received) == ERROR)
        return false;

    std::stringstream ss;
    ss.read(received, msg_length);
    peertable.read_stream(ss);

    free(received);
    return true;
}