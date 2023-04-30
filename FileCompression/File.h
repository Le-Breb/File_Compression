#pragma once
#include <tuple>

#include "LFH.h"

class File
{
public:
    char* compressed_data;
    LFH lfh;
    int size_with_header;

    File(const LFH& lfh, char* compressed_data);
    ~File();
    explicit File(const char* path);
    char* to_bytes() const;
    const std::tuple<char*, int> get_compressed_data(char* data, int file_size) const;
    const std::tuple<char*, int> get_uncompressed_data(char* data, int compressed_file_size) const;
    //char* to_bytes(const CDFH& cdfh) const;
};
