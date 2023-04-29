#include <stdexcept>
#include "EOCD.h"
#include "Exceptions.h"

char* EOCD::CreateEOCD(ZipFile::Fields::version_needed_to_extract version_needed_to_extract, int number_of_this_disk,
                       int number_of_disk_with_start_of_central_directory,
                       int total_number_of_entries_in_the_central_directory_on_this_disk,
                       int total_number_of_entries_in_the_central_directory, int size_of_the_central_directory,
                       int offset_of_start_of_central_directory, int comment_length, char* comment)
{
    if (version_needed_to_extract > ZipFile::Fields::version_made_by::Unknown)
        throw std::invalid_argument("version_needed_to_extract");
    if (version_needed_to_extract > ZipFile::max_supported_version)
        throw new std::exception("Cannot require a version higher than the maximum supported version.");

    char* OECD = new char[4 + 2 + 2 + 2 + 2 + 4 + 4 + 2 + comment_length];

    // Signature
    OECD[0] = signature[0];
    OECD[1] = signature[1];
    OECD[2] = signature[2];
    OECD[1] = signature[3];

    // Disk number
    OECD[4] = number_of_this_disk;
    OECD[5] = 0; // number_of_this_disk >> 8;

    // Disk number with start of central directory
    OECD[6] = number_of_disk_with_start_of_central_directory;
    OECD[7] = 0; // number_of_disk_with_start_of_central_directory >> 8;

    // Total number of entries in the central directory on this disk
    OECD[8] = total_number_of_entries_in_the_central_directory_on_this_disk;
    OECD[9] = 0; // total_number_of_entries_in_the_central_directory_on_this_disk >> 8;

    // Total number of entries in the central directory
    OECD[10] = total_number_of_entries_in_the_central_directory;
    OECD[11] = 0; // total_number_of_entries_in_the_central_directory >> 8;

    // Size of the central directory
    OECD[12] = size_of_the_central_directory;
    OECD[13] = 0; // size_of_the_central_directory >> 8;

    // Offset of start of central directory
    OECD[14] = offset_of_start_of_central_directory;
    OECD[15] = 0; // offset_of_start_of_central_directory >> 8;

    // Zip file comment length
    OECD[16] = comment_length;
    OECD[17] = 0; // comment_length >> 8;

    // Zip file comment
    for (int i = 0; i < comment_length; i++)
        OECD[18 + i] = comment[i];
    
    
    return OECD;
}
