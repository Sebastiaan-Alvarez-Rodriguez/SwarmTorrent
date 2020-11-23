#include <iostream>
#include <tclap/CmdLine.h>
#include <stdexcept>

#include "peerTable/peerTable.h"
#include "shared/util/print.h"


bool setup(int argc, char const ** argv, IPTable& pt) {
    TCLAP::CmdLine cmd("SwarmTorrent Tracker Main", ' ', "0.1");
    TCLAP::ValueArg<std::string> seederArg("i", "initial-seeder", "IP of initial seeder, format: [TCP]:[4/6]:PORT:IP", true, "", "IP Strings", cmd);

    cmd.parse(argc, argv);

    std::string initial_seeder = seederArg.getValue();
    try {
        pt = peertable::init(initial_seeder);
    } catch (const std::exception& e) {
        std::cerr << print::RED << "[ERROR] " << e.what() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char const **argv) {
    IPTable pt;
    if (!setup(argc, argv, pt))
        return 1;
    //TODO: run() or listen() or anything else
    return 0;
}