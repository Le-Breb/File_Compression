#pragma once
#include <tuple>

#include "CDFH.h"

class File
{
public:
    char* compressed_data;
    
    ZipFile::Fields::version_needed_to_extract version_needed_to_extract;
    ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag;
    ZipFile::Fields::compression_method compression_method;
    short last_mod_file_time;
    short last_mod_file_date;
    int crc32;
    int compressed_size;
    int uncompressed_size;
    short file_name_length;
    short extra_field_length;
    short file_comment_length;
    short disk_number_start;
    short internal_file_attributes;
    int external_file_attributes;
    const char* file_name;
    const char* extra_field;
    const char* file_comment;

    ~File();
    explicit File(std::ifstream& in, const CDFH& cdfh);
    explicit File(const char* path, ZipFile::Fields::compression_method compression_method = ZipFile::Fields::compression_method::Stored);
    std::tuple<char*, int> get_compressed_data(char* data, int file_size) const;
    const std::tuple<char*, int> get_uncompressed_data(char* data, int compressed_file_size) const;
    //char* to_bytes(const CDFH& cdfh) const;
};
