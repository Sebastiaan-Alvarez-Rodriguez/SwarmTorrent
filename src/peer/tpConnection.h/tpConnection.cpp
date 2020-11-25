#include <sstream>
#include <cstring>

#include "tpConnection.h"
#include "shared/connection/tracker/trackerConnection.h"

static void request(std::unique_ptr<Connection> connection, Header header, size_t msg_length, void* const msg) {
    void* const buf = malloc(sizeof(Header) + msg_length);
    memcpy(buf, &header, sizeof(Header));
    memcpy((uint8_t*)buf + sizeof(Header), msg, msg_length);

    connection->sendmsg(buf, sizeof(Header)+msg_length);
    free(buf);
}

static STATUS receive(std::unique_ptr<Connection> connection, size_t& msg_length, void* const msg) {
    Header header;
    connection->recvmsg(&header, sizeof(Header));

    if (header.status == ERROR)
        return ERROR; 

    msg_length = header.size;
    msg = malloc(msg_length);
    connection->recvmsg(msg, msg_length);

    return header.status;
}

bool subscribe(std::unique_ptr<Connection> connection, std::string torrent_hash, Addr peer) {    std::stringstream hstream; 
    peer.write_stream(hstream);
    size_t msg_length = hstream.str().size(); 
    Header header = {SUBSCRIBE, REQUEST, msg_length};
    char* msg = hstream.c_str();

    request(connection, header, msg_length, msg);

    if (receive(connection, msg_length, msg) == ERROR)
        return false;

    free(msg);
    return true;
}

bool unsubscribe(std::unique_ptr<Connection> connection, std::string torrent_hash, Addr peer) {

}

bool retrieve(std::unique_ptr<Connection> connection, std::string torrent_hash, IPTable& peertable) {

}