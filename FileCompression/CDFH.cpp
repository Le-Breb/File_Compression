#include "CDFH.h"

#include <cstring>
#include <exception>

CDFH::CDFH(ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
    ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
    ZipFile::Fields::compression_method compression_method, short last_mod_file_time, short last_mod_file_date,
    int crc32, int compressed_size, int uncompressed_size, short file_name_length, short extra_field_length,
    short file_comment_length, short disk_number_start, short internal_file_attributes, int external_file_attributes,
    int relative_offset_of_local_header, const char* file_name, const char* extra_field, const char* file_comment) :
    version_needed_to_extract(version_needed_to_extract), general_purpose_bit_flag(general_purpose_bit_flag),
    compression_method(compression_method), last_mod_file_time(last_mod_file_time),
    last_mod_file_date(last_mod_file_date), crc32(crc32), compressed_size(compressed_size),
    uncompressed_size(uncompressed_size), file_name_length(file_name_length), extra_field_length(extra_field_length),
    file_comment_length(file_comment_length), disk_number_start(disk_number_start),
    internal_file_attributes(internal_file_attributes), external_file_attributes(external_file_attributes),
    relative_offset_of_local_header(relative_offset_of_local_header), file_name(file_name), extra_field(extra_field),
    file_comment(file_comment), byte_size(46 + file_name_length + extra_field_length + file_comment_length)
{
}

CDFH::CDFH(const LFH& lfh, short file_comment_length, short disk_number_start, short internal_file_attributes,
    int external_file_attributes, int relative_offset_of_local_header, const char* file_comment) :
    version_needed_to_extract(lfh.version_needed_to_extract), general_purpose_bit_flag(lfh.general_purpose_bit_flag),
    compression_method(lfh.compression_method), last_mod_file_time(lfh.last_mod_file_time),
    last_mod_file_date(lfh.last_mod_file_date), crc32(lfh.crc32), compressed_size(lfh.compressed_size),
    uncompressed_size(lfh.uncompressed_size), file_name_length(lfh.file_name_length),
    extra_field_length(lfh.extra_field_length), file_comment_length(file_comment_length),
    disk_number_start(disk_number_start), internal_file_attributes(internal_file_attributes),
    external_file_attributes(external_file_attributes),
    relative_offset_of_local_header(relative_offset_of_local_header), file_name(lfh.file_name),
    extra_field(lfh.extra_field), file_comment(file_comment),
    byte_size(46 + file_name_length + extra_field_length + file_comment_length)
{
    
}

char* CDFH::to_bytes() const
{
    char* buffer = new char[byte_size];
    
    memcpy(buffer, signature, 4);
    memcpy(buffer + 4, &version_needed_to_extract, 2);
    memcpy(buffer + 6, &general_purpose_bit_flag, 2);
    memcpy(buffer + 8, &compression_method, 2);
    memcpy(buffer + 10, &last_mod_file_time, 2);
    memcpy(buffer + 12, &last_mod_file_date, 2);
    memcpy(buffer + 14, &crc32, 4);
    memcpy(buffer + 18, &compressed_size, 4);
    memcpy(buffer + 22, &uncompressed_size, 4);
    memcpy(buffer + 26, &file_name_length, 2);
    memcpy(buffer + 28, &extra_field_length, 2);
    memcpy(buffer + 30, &file_comment_length, 2);
    memcpy(buffer + 32, &disk_number_start, 2);
    memcpy(buffer + 34, &internal_file_attributes, 2);
    memcpy(buffer + 36, &external_file_attributes, 4);
    memcpy(buffer + 40, &relative_offset_of_local_header, 4);
    memcpy(buffer + 44, file_name, file_name_length);
    memcpy(buffer + 44 + file_name_length, extra_field, extra_field_length);
    memcpy(buffer + 44 + file_name_length + extra_field_length, file_comment, file_comment_length);
    
    return buffer;
}

CDFH::~CDFH()
{
    delete[] file_name;
    delete[] extra_field;
    delete[] file_comment;
}
