#include <iostream>
#include <fstream>
#include <limits>

#include "filesystem.h"

unsigned file_size(const std::string& path) {
    //https://stackoverflow.com/questions/22984956/tellg-function-give-wrong-size-of-file/22986486#22986486
    std::ifstream file(path, std::ios::binary);
    file.ignore(std::numeric_limits<std::streamsize>::max());
    unsigned length = (unsigned)file.gcount();
    file.clear();
    file.close();
    return length;
}

std::string basename(const std::string& path) { 
    size_t sepPos = path.rfind('/');
    if (sepPos == std::string::npos)
        return "";
    return path.substr(sepPos+1, path.size()-1);
}

std::string append(const std::string& p1, const std::string& p2) {
    const char sep = '/';
    if (p1[p1.length()-1] != sep)
        return p1 + sep + p2;
    return p1 + p2;
}