#pragma once
#include "ZipFile.h"

/**
 * \brief Local file header
 */
class LFH
{
public:
    static constexpr unsigned int signature = 0x04034b50;

    ZipFile::Fields::version_needed_to_extract version_needed_to_extract;
    ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag;
    ZipFile::Fields::compression_method compression_method;
    unsigned short last_mod_file_time;
    unsigned short last_mod_file_date;
    unsigned int crc32;
    unsigned int compressed_size;
    unsigned int uncompressed_size;
    unsigned short file_name_length;
    unsigned short extra_field_length;
    char* file_name;
    char* extra_field;
    unsigned int byte_size;

    LFH(ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
        ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
        ZipFile::Fields::compression_method compression_method = ZipFile::Fields::compression_method::Stored,
        unsigned short last_mod_file_time = 0, unsigned short last_mod_file_date = 0, unsigned crc32 = 0, unsigned int compressed_size = 0,
        unsigned int uncompressed_size = 0, unsigned short file_name_length = 0, unsigned short extra_field_length = 0,
        char* file_name = nullptr, char* extra_field = nullptr);

    explicit LFH(std::ifstream& in, unsigned offset);

    ~LFH();

    static std::tuple<char*, int> build_from(const File& file);

    char* to_bytes() const;
};
