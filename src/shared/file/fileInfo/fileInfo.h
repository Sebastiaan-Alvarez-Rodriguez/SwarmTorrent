#ifndef FILEINFO_H
#define FILEINFO_H 

#include <vector>

#include "../torrent/torrentFile.h"
#include "../../../util/filesystem.h"

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)

class FileInfo {
public:
    //Constructor used for the initial seeder
    //Splits the file into fragments of size fragment_size (in bytes)
    //Seed threshold will be number of fragments / divide_threshold
    //Creates a torrentfile associated with the file
    FileInfo(std::string path, unsigned fragment_size, unsigned divide_threshold, TrackerTable trackertable);
    //Constructor used for the initial seeder
    //Splits the file into fragments of the default fragment_size
    //Creates a torrentfile associated with the file
    FileInfo(std::string path, TrackerTable trackertable) : FileInfo(path, MB(1), 20, trackertable) {};
    //Constructor used for a peer who just retrieved a TorrentFile
    //path is the path to the directory in which to place the file
    FileInfo(TorrentFile& torrentfile, std::string path) : torrentfile(torrentfile), received(torrentfile.get_nr_fragments(), false), path(path) {};
    //Constructor used for a peer who just retrieved a TorrentFile
    //path is the path to the directory in which to place the file
    //filename is the name of the file to create
    FileInfo(TorrentFile& torrentfile, std::string path, std::string filename) : torrentfile(torrentfile), received(torrentfile.get_nr_fragments(), false), path(append(path, filename)) {};

    //Returns true if a fragment at index has been received, else false
    bool has_fragment(unsigned index) const {return received.at(index);};
    //Set the fragment status of fragment at index to received
    void set_received(unsigned index) {received.at(index) = true; ++nr_fragments;};
    //Returns a constant reference to the torrentfile
    const TorrentFile& get_torrentfile() const {return torrentfile;};
    //Returns the path to write fragments to
    const std::string& get_path() const {return path;};
    //Returns the number of fragments received
    unsigned get_nr_fragments() const {return nr_fragments;};

private:
    //TorrentFile associated with file
    TorrentFile torrentfile;
    //Identifies if a fragment has been recieved
    std::vector<bool> received;
    //Path to write fragments to
    std::string path;
    //Number of received fragments
    unsigned nr_fragments = 0;
};

#endif