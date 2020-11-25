#include "trackerConnection.h"

// enum TAG {
//     SUBSCRIBE = 0, 
//     UNSUBSCRIBE, 
//     RECEIVE, 
//     UPDATE
// };

// enum STATUS {
//     REQUEST = 0, 
//     CONFIRM, 
//     ERROR
// };

// struct Header {
//     TAG tag;
//     STATUS status;  
//     size_t size;
// };

static uint8_t pack(TAG tag, STATUS status) {
    

}

static void unpack(TAG& tag, STATUS& status, uint8_t byte) {


}

static uint8_t* serialize(const Header& header) {
    //sizeof(size_t)


}

static Header deserialize(const uint8_t* const ptr) {

}

void send_header(std::unique_ptr<Connection> connection, const Header& header) {


}

void receive_header(std::unique_ptr<Connection> connection, Header& header) {


}