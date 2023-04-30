#pragma once
#include "ZipFile.h"

/**
 * \brief End of central directory record
 */
class EOCD
{
public:
    static constexpr int signature = 0x06054b50;

    static void CreateEOCD(char* buffer, ZipFile::Fields::version_needed_to_extract version_needed_to_extract = ZipFile::Fields::version_needed_to_extract::v4_6,
        short number_of_this_disk = ZipFile::disk_number,
        short number_of_disk_with_start_of_central_directory = ZipFile::disk_number,
        short total_number_of_entries_in_the_central_directory_on_this_disk = 1,
        short total_number_of_entries_in_the_central_directory = 1,
        int size_of_the_central_directory = 0,
        int offset_of_start_of_central_directory = 0,
        short zip_file_comment_length = 0,
        char* zip_file_comment = nullptr);
};
