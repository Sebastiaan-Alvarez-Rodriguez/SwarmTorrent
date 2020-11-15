#include "fileInfo.h"

FileInfo::FileInfo(std::string path, unsigned fragment_size, unsigned divide_threshold) : torrentfile(basename(path), file_size(path), fragment_size, (((file_size(path) -1) / fragment_size) + 1) / divide_threshold), path(path) {
    unsigned size = file_size(path);
    unsigned nr_fragments = ((size -1) / fragment_size) + 1;
    received.assign(nr_fragments, true);
    this->nr_fragments = nr_fragments;
}

