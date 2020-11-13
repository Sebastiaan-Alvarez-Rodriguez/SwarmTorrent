#ifndef TORRENTFILE_H
#define TORRENTFILE_H

#include <string>


// Object responsible for reading and writing (.st) torrent files
class TorrentFile {
public:
    /** Constructs torrentfile by providing a path. Path should point to a .st file */
    TorrentFile(std::string path);
    ~TorrentFile();
    
    // TODO: Decide whether we let other objects decide when to read...
    //      Proposal: Read all data in data structures at ones.
    //      Reason: Users can delete .torrent files after it has been read
    // bool read();
    // bool write();

    // TODO: Write members here to fetch data parts (e.g. "get checksum for file X, chunk Y", get trackerlist)

protected:
    std::string path;
};
#endif