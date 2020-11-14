#ifndef CONNECTION_TYPE_H
#define CONNECTION_TYPE_H 

#include <sys/socket.h> 

// A simple start
// (TODO: Fetch integer values for types and assign them to enum vals)
// https://www.geeksforgeeks.org/socket-programming-cc/
enum ConnectionType {
    AF_INET_T, // (IPv4 protocol)
    AF_INET6_T, // (IPv6 protocol)
    SOCK_STREAM_T, // TCP
    SOCK_DGRAM_T // UDP
};
#endif