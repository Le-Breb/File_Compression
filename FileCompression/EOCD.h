#pragma once
#include "ZipFile.h"

/**
 * \brief End of central directory record
 */
class EOCD
{
public:
    static constexpr int signature = 0x06054b50;

    unsigned short number_of_this_disk;
    unsigned short number_of_disk_with_start_of_central_directory;
    unsigned short total_number_of_entries_in_the_central_directory_on_this_disk;
    unsigned short total_number_of_entries_in_the_central_directory;
    unsigned int size_of_the_central_directory;
    unsigned int offset_of_start_of_central_directory;
    unsigned short zip_file_comment_length;
    char* zip_file_comment;
    unsigned int byte_size;

    EOCD(unsigned short number_of_this_disk = ZipFile::disk_number,
        unsigned short number_of_disk_with_start_of_central_directory = ZipFile::disk_number,
        unsigned short total_number_of_entries_in_the_central_directory_on_this_disk = 0,
        unsigned short total_number_of_entries_in_the_central_directory = 0,
        unsigned int size_of_the_central_directory = 0,
        unsigned int offset_of_start_of_central_directory = 0,
        unsigned short zip_file_comment_length = 0,
        char* zip_file_comment = nullptr);

    explicit EOCD(std::ifstream& file, unsigned int offset);

    ~EOCD();

    char* to_bytes() const;
    
    friend std::ostream& operator<<(std::ostream& os, const EOCD& eocd);
};
