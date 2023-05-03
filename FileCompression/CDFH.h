#pragma once
#include "LFH.h"
#include "ZipFile.h"
#include "MS-DOS/Date.h"
#include "MS-DOS/Time.h"

/**
 * \brief Central directory file header
 */
class CDFH
{
public:
    static constexpr int signature = 0x02014b50;
    
    ZipFile::Fields::version_made_by version_made_by;
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
    unsigned int relative_offset_of_local_header;
    char* file_name;
    char* extra_field;
    char* file_comment;
    unsigned int byte_size;

    CDFH(ZipFile::Fields::version_made_by version_made_by, ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
                                  ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
                                  ZipFile::Fields::compression_method compression_method, const MS_DOS::Time& last_mod_file_time,
                                  const MS_DOS::Date& last_mod_file_date, unsigned int crc32 = 0, unsigned int compressed_size = 0,
                                  unsigned int uncompressed_size = 0, unsigned short file_name_length = 0, unsigned short extra_field_length = 0,
                                  unsigned short file_comment_length = 0, unsigned short disk_number_start = ZipFile::disk_number,
                                  unsigned short internal_file_attributes = 0, unsigned int external_file_attributes = 0,
                                  unsigned int relative_offset_of_local_header = 0, char* file_name = nullptr,
                                  char* extra_field = nullptr, char* file_comment = nullptr);

    explicit CDFH(const LFH& lfh, unsigned short file_comment_length = 0, unsigned short disk_number_start = ZipFile::disk_number, unsigned short internal_file_attributes = 0, unsigned int external_file_attributes = 0, unsigned int relative_offset_of_local_header = 0, char* file_comment = nullptr);

    explicit CDFH(std::ifstream& in, int offset);

    char* to_bytes() const;

    static std::tuple<char*, int> build_from(const File& file, const int relative_offset_of_local_header);

    ~CDFH();
};
