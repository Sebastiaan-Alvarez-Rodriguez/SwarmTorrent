#include <cstdint>
#include <fstream>
#include <limits>
#include "fs_compat.h"

#include "fs.h"

uint64_t fs::file_size(const std::string& path) {
    return std::filesystem::file_size(path);
}

//TODO: replace by filesystem function
std::string fs::basename(const std::string& path) { 
    return std::filesystem::path(path).filename(); 
}

std::string fs::append(const std::string& p1, const std::string& p2) {
    return (std::filesystem::path(p1) / p2).string();
}