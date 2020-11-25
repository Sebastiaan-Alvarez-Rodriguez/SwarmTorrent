#ifndef TP_CONNECTION_TRACKER_H
#define TP_CONNECTION_TRACKER_H

#include "shared/connection/tracker/trackerConnection.h"

TAG receive(std::unique_ptr<Connection> connection, void* const message);
void response(std::unique_ptr<Connection> connection, TAG tag, STATUS status, void* const message);

#endif