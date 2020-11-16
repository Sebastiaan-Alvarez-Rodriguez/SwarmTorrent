#include <cstdint>
#include <fstream>
#include <limits>
#include "fs_compat.h"

#include "fs.h"

uint64_t fs::file_size(const std::string& path) {
    return std::filesystem::file_size(path);
}

std::string fs::basename(const std::string& path) { 
    size_t sepPos = path.rfind('/');
    if (sepPos == std::string::npos)
        return "";
    return path.substr(sepPos+1, path.size()-1);
}

std::string fs::append(const std::string& p1, const std::string& p2) {
    const char sep = '/';
    if (p1[p1.length()-1] != sep)
        return p1 + sep + p2;
    return p1 + p2;
}