#include "LFH.h"

#include <cstring>
#include "File.h"

LFH::LFH(ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
         ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
         ZipFile::Fields::compression_method compression_method, short last_mod_file_time, short last_mod_file_date,
         int crc32, int compressed_size, int uncompressed_size, short file_name_length, short extra_field_length,
         const char* file_name, const char* extra_field) : version_needed_to_extract(version_needed_to_extract),
                                                           general_purpose_bit_flag(general_purpose_bit_flag),
                                                           compression_method(compression_method),
                                                           last_mod_file_time(last_mod_file_time),
                                                           last_mod_file_date(last_mod_file_date), crc32(crc32),
                                                           compressed_size(compressed_size),
                                                           uncompressed_size(uncompressed_size),
                                                           file_name_length(file_name_length),
                                                           extra_field_length(extra_field_length), file_name(file_name),
                                                           extra_field(extra_field),
                                                           byte_size(30 + file_name_length + extra_field_length)
{
}

LFH::~LFH()
{
    delete[] file_name;
    delete[] extra_field;
}

std::tuple<char*, int> LFH::build_from(const File& file)
{
    int byte_size = 30 + file.file_name_length + file.extra_field_length;
    char* bytes = new char[byte_size];
    memcpy(bytes, &signature, 4);
    memcpy(bytes + 4, &file.version_needed_to_extract, 2);
    memcpy(bytes + 6, &file.general_purpose_bit_flag, 2);
    memcpy(bytes + 8, &file.compression_method, 2);
    memcpy(bytes + 10, &file.last_mod_file_time, 2);
    memcpy(bytes + 12, &file.last_mod_file_date, 2);
    memcpy(bytes + 14, &file.crc32, 4);
    memcpy(bytes + 18, &file.compressed_size, 4);
    memcpy(bytes + 22, &file.uncompressed_size, 4);
    memcpy(bytes + 26, &file.file_name_length, 2);
    memcpy(bytes + 28, &file.extra_field_length, 2);
    memcpy(bytes + 30, file.file_name, file.file_name_length);
    memcpy(bytes + 30 + file.file_name_length, file.extra_field, file.extra_field_length);
    
    return std::tuple<char*, int>(bytes, byte_size);
}

char* LFH::to_bytes() const
{
    char* bytes = new char[byte_size];
    
    memcpy(bytes, &signature, 4);
    memcpy(bytes + 4, &version_needed_to_extract, 2);
    memcpy(bytes + 6, &general_purpose_bit_flag, 2);
    memcpy(bytes + 8, &compression_method, 2);
    memcpy(bytes + 10, &last_mod_file_time, 2);
    memcpy(bytes + 12, &last_mod_file_date, 2);
    memcpy(bytes + 14, &crc32, 4);
    memcpy(bytes + 18, &compressed_size, 4);
    memcpy(bytes + 22, &uncompressed_size, 4);
    memcpy(bytes + 26, &file_name_length, 2);
    memcpy(bytes + 28, &extra_field_length, 2);
    memcpy(bytes + 30, file_name, file_name_length);
    memcpy(bytes + 30 + file_name_length, extra_field, extra_field_length);
    
    return bytes;
}

