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
    MS_DOS::Time* last_mod_file_time;
    MS_DOS::Date* last_mod_file_date;
    unsigned int crc32;
    unsigned int compressed_size;
    unsigned int uncompressed_size;
    unsigned short file_name_length;
    unsigned short extra_field_length;
    unsigned short file_comment_length;
    unsigned short disk_number_start;
    unsigned short internal_file_attributes;
    unsigned int external_file_attributes;
    const char* file_name;
    const char* extra_field;
    const char* file_comment;

    ~File();

    explicit File(std::ifstream& in, const CDFH& cdfh);

    explicit File(const std::string& path_on_disk, ZipFile::Fields::compression_method compression_method,
                  const MS_DOS::Time& last_mod_file_time, const MS_DOS::Date& last_mod_file_date,
                  const bool is_apparently_text, const unsigned int external_attributes,
                  const std::string& path_in_zip);

    std::pair<char*, int> get_compressed_data(const char* data, int file_size);

    std::pair<char*, int> get_uncompressed_data(char* data, int compressed_file_size) const;

    friend std::ostream& operator<<(std::ostream& os, const File& file);
    //char* to_bytes(const CDFH& cdfh) const;
};
