#ifndef CONNECTION_FACTORY_H
#define CONNECTION_FACTORY_H

#include "shared/connection/meta/type.h"
class ConnectionFactory {
public:
    ConnectionFactory();
    ~ConnectionFactory();
    
    ConnectionFactory withType(ConnectionType type);
};
#endif