#pragma once
#include "ZipFile.h"

/**
 * \brief Local file header
 */
class LFH
{
public:
    static constexpr int signature = 0x04034b50;

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
    const char* file_name;
    const char* extra_field;
    int byte_size;

    LFH(ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
        ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
        ZipFile::Fields::compression_method compression_method = ZipFile::Fields::compression_method::Stored,
        short last_mod_file_time = 0, short last_mod_file_date = 0, int crc32 = 0, int compressed_size = 0,
        int uncompressed_size = 0, short file_name_length = 0, short extra_field_length = 0,
        const char* file_name = nullptr, const char* extra_field = nullptr);

    ~LFH();

    char* to_bytes() const;
};
