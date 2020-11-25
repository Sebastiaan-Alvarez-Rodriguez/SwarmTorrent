#include <iostream>
#include <tclap/CmdLine.h>
#include <stdexcept>

#include "shared/connection/impl/TCP/in/TCPInConnection.h"
#include "shared/torrent/ipTable/ipTable.h"
#include "shared/util/print.h"
#include "tracker.h"

int main(int argc, char const **argv) {
    Tracker tracker; 
    std::cout << "I am a tracker!" << std::endl;
    return 0;
}