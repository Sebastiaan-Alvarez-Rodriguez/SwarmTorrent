#ifndef FILESYSTEM_H
#define FILESYSTEM_H 

#include <string>
//Warning: the functions in this file perform operations based on the UNIX filesystem

//Returns the file size in bytes
unsigned file_size(const std::string& path);
//Returns the basename of a path (extension inclusive)
std::string basename(const std::string& path);
//Appends two paths 
std::string append(const std::string& p1, const std::string& p2);


#endif