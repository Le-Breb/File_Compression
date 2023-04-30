#pragma once
#include "ZipFile.h"

/**
 * \brief Central directory file header
 */
class CDFH
{
public:
    static constexpr char signature[] = "PK\1\2";
    
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
    int relative_offset_of_local_header;
    const char* file_name;
    const char* extra_field;
    const char* file_comment;
    int byte_size;

    CDFH(ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
                                  ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
                                  ZipFile::Fields::compression_method compression_method =
                                      ZipFile::Fields::compression_method::Stored, short last_mod_file_time = 0,
                                  short last_mod_file_date = 0, int crc32 = 0, int compressed_size = 0,
                                  int uncompressed_size = 0, short file_name_length = 0, short extra_field_length = 0,
                                  short file_comment_length = 0, short disk_number_start = ZipFile::disk_number,
                                  short internal_file_attributes = 0, int external_file_attributes = 0,
                                  int relative_offset_of_local_header = 0, const char* file_name = nullptr,
                                  const char* extra_field = nullptr, const char* file_comment = nullptr);

    char* to_bytes() const;

    ~CDFH();
};
