#include <iostream>
#include <tclap/CmdLine.h>

bool setup(int argc, char const ** argv) {
    TCLAP::CmdLine cmd("SwarmTorrent Tracker Main", ' ', "0.1");
    TCLAP::ValueArg<std::string> seederArg("i", "initial-seeder", "IP of initial seeder, format: [TCP]:[4/6]:PORT:IP", true, "IP Strings", cmd);

    cmd.parse(argc, argv);

    std::string initial_seeder = seederArg.getValue();

}

int main(int argc, char const **argv) {
    if (!setup(argc, argv))
        return 1;
    //TODO: run() or listen() or anything else
    return 0;
}