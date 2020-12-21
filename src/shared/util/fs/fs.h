#ifndef FS_H
#define FS_H 

#include <cstdint>
#include <string>
#include "fs_compat.h"


namespace fs {
    // Returns the file size in bytes
    uint64_t file_size(const std::string& path);

    // Returns the basename of a path (extension inclusive)
    std::string basename(const std::string& path);

    // Returns if given path is a file
    inline bool is_file(const std::string& path) {
        return std::filesystem::is_regular_file(path);
    }

    // Returns is given path is a dir
    inline bool is_dir(const std::string& path) {
        return std::filesystem::is_directory(path);
    }

    // Construct a series of directories. Returns true on success, false on failure
    inline bool mkdir(const std::string& path) {
        return std::filesystem::create_directories(path);
    }
    
    // Appends two paths 
    std::string append(const std::string& p1, const std::string& p2);
}

#endif