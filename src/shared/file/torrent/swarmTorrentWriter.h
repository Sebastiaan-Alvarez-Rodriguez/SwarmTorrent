#ifndef SWARMTORRENTWRITER_H
#define SWARMTORRENTWRITER_H 

class SwarmTorrentWriter {
public:
    virtual void write_swarm(std::ostream& os) const = 0;
    virtual void read_swarm(std::istream& is) = 0;
    
};


#endif