#include "CD.h"

#include <cstring>
#include <exception>

char* CD::create_cd_header(char* buffer, ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
                           ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
                           ZipFile::Fields::compression_method compression_method, short last_mod_file_time, short last_mod_file_date,
                           int crc32, int compressed_size, int uncompressed_size, short file_name_length, short extra_field_length,
                           short file_comment_length, short disk_number_start, short internal_file_attributes, int external_file_attributes,
                           int relative_offset_of_local_header, const char* file_name, const char* extra_field, const char* file_comment)
{
    // Signature
    memcpy(buffer, signature, sizeof signature - 1);
    buffer += sizeof signature - 1;

    // Version made by
    *buffer++ = ZipFile::current_version;

    throw std::exception("Not implemented");
}
