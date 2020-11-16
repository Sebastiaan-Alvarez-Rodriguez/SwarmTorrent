// Simple preprocesor hack to find out whether to use
// <filesystem> or <experimental/filesystem>

// We haven't checked which filesystem to include yet
#ifndef HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM
#   if defined(__cpp_lib_filesystem) // Feature test macro for <filesystem>
#       define HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM 0
#   elif defined(__cpp_lib_experimental_filesystem) // Feature test macro for <experimental/filesystem>
#       define HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM 1
#   elif !defined(__has_include) // If we cannot check if headers exist, use experimental
#       define HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM 1
#   elif __has_include(<filesystem>) // Check if the header "<filesystem>" exists
#       define HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM 0
#   elif __has_include(<experimental/filesystem>) // Check if the header "<filesystem>" exists
#       define HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM 1
#   else // Cannot get any filesystem library
#       error Could not find system header "<filesystem>" or "<experimental/filesystem>"
#   endif
#   if HACKS_USE_STD_EXPERIMENTAL_FILESYSTEM
#       include <experimental/filesystem>
    // Move namespace filesystem to experimental::filesystem;
    namespace std {
        namespace filesystem = experimental::filesystem;
    }
#   else
#       include <filesystem>
#   endif
#endif