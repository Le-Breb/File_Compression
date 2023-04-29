#pragma once
#include "ZipFile.h"

/**
 * \brief End of central directory record
 */
class EOCD
{
public:
    static constexpr char signature[] = "PK\3\4";
    static constexpr int disk_number = 1;

    static char* CreateEOCD(ZipFile::Fields::version_needed_to_extract version_needed_to_extract = ZipFile::Fields::version_needed_to_extract::v4_6,
        int number_of_this_disk = disk_number,
        int number_of_disk_with_start_of_central_directory = disk_number,
        int total_number_of_entries_in_the_central_directory_on_this_disk = 1,
        int total_number_of_entries_in_the_central_directory = 1,
        int size_of_the_central_directory = 0,
        int offset_of_start_of_central_directory = 0,
        int zip_file_comment_length = 0,
        char* zip_file_comment = nullptr);
};
