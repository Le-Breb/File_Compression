#include <stdexcept>
#include "EOCD.h"
#include "Exceptions.h"

void EOCD::CreateEOCD(char* buffer, ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
    short number_of_this_disk, short number_of_disk_with_start_of_central_directory,
    short total_number_of_entries_in_the_central_directory_on_this_disk,
    short total_number_of_entries_in_the_central_directory, int size_of_the_central_directory,
    int offset_of_start_of_central_directory, short zip_file_comment_length, char* zip_file_comment)
{
    // Signature
    memcpy(buffer, &signature, sizeof (int));
    buffer += sizeof(int);

    // Number of this disk
    memcpy(buffer, &number_of_this_disk, sizeof (short));
    buffer += sizeof(short);

    // Disk where CD starts
    memcpy(buffer, &number_of_disk_with_start_of_central_directory, sizeof (short));

    // Number of entries in the CD on this disk
    memcpy(buffer, &total_number_of_entries_in_the_central_directory_on_this_disk, sizeof (short));
    buffer += sizeof(short);

    // Total number of entries in the CD
    memcpy(buffer, &total_number_of_entries_in_the_central_directory, sizeof (short));
    buffer += sizeof(short);

    // Size of the CD
    memcpy(buffer, &size_of_the_central_directory, sizeof (int));
    buffer += sizeof(int);

    // Offset of the CD
    memcpy(buffer, &offset_of_start_of_central_directory, sizeof (int));
    buffer += sizeof(int);

    // Zip file comment length
    memcpy(buffer, &zip_file_comment_length, sizeof (short));
    buffer += sizeof(short);

    // Zip file comment
    if (zip_file_comment_length > 0)
    {
        memcpy(buffer, zip_file_comment, zip_file_comment_length);
        buffer += zip_file_comment_length * sizeof(char);
    }
}
