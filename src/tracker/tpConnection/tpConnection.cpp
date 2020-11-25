#include <sstream>
#include <cstring>

#include "tpConnection.h"

TAG peek_request(std::unique_ptr<Connection>& connection) {
    Header header; 
    connection->peekmsg(&header, sizeof(header));
    return header.tag;
}

bool get_member(std::unique_ptr<Connection>& connection, std::string& hash, Addr& peer) {
    Header header; 
    connection->recvmsg(&header, sizeof(Header));
    if (header.status == ERROR)
        return false;

    size_t msg_length = header.size;
    char* const msg = (char*)malloc(msg_length);
    connection->recvmsg(msg, msg_length);

    //deserialize torrent_hash and peer
    std::stringstream ss;
    ss.read(msg, msg_length);
    size_t hash_length; 
    ss >> hash_length;
    hash.resize(hash_length);
    ss.read((char*)hash.data(), hash_length);
    peer.read_stream(ss);

    free(msg);
    return true;
}

bool get_table(std::unique_ptr<Connection>& connection, std::string& hash) {
    Header header; 
    connection->recvmsg(&header, sizeof(Header));
    if (header.status == ERROR)
        return false;

    connection->recvmsg(hash.data(), header.size);
    return true;
}

static void send(std::unique_ptr<Connection>& connection, Header header, size_t msg_length, const void* const msg) {
    void* const buf = malloc(sizeof(Header) + msg_length);
    memcpy(buf, &header, sizeof(Header));
    memcpy((uint8_t*)buf + sizeof(Header), msg, msg_length);

    connection->sendmsg(buf, sizeof(Header)+msg_length);
    free(buf);
}

void send_table(std::unique_ptr<Connection>& connection, IPTable& peertable) {
    std::stringstream ss;
    peertable.write_stream(ss);
    std::string tmp = ss.str();
    Header header = {RECEIVE, CONFIRM, tmp.size()};
    const char* msg = tmp.c_str();
    send(connection, header, tmp.size(), msg);
}

void send_response(std::unique_ptr<Connection>& connection, TAG tag, STATUS status) {
    Header header = {tag, status, 0};
    connection->sendmsg(&header, sizeof(Header));
}
