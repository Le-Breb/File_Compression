#include <stdexcept>
#include "EOCD.h"
#include "Exceptions.h"

EOCD::EOCD(ZipFile::Fields::version_needed_to_extract version_needed_to_extract, short number_of_this_disk,
    short number_of_disk_with_start_of_central_directory,
    short total_number_of_entries_in_the_central_directory_on_this_disk,
    short total_number_of_entries_in_the_central_directory, int size_of_the_central_directory,
    int offset_of_start_of_central_directory, short zip_file_comment_length, const char* zip_file_comment) :
    version_needed_to_extract(version_needed_to_extract), number_of_this_disk(number_of_this_disk),
    number_of_disk_with_start_of_central_directory(number_of_disk_with_start_of_central_directory),
    total_number_of_entries_in_the_central_directory_on_this_disk(
        total_number_of_entries_in_the_central_directory_on_this_disk),
    total_number_of_entries_in_the_central_directory(total_number_of_entries_in_the_central_directory),
    size_of_the_central_directory(size_of_the_central_directory),
    offset_of_start_of_central_directory(offset_of_start_of_central_directory),
    zip_file_comment_length(zip_file_comment_length), zip_file_comment(zip_file_comment)
{
}

EOCD::~EOCD()
{
    delete[] zip_file_comment;
}

char* EOCD::to_bytes() const
{
    //
    char* bytes = new char[22 + zip_file_comment_length];

    memcpy(bytes, &signature, 4);
    memcpy(bytes + 4, &number_of_this_disk, 2);
    memcpy(bytes + 6, &number_of_disk_with_start_of_central_directory, 2);
    memcpy(bytes + 8, &total_number_of_entries_in_the_central_directory_on_this_disk, 2);
    memcpy(bytes + 10, &total_number_of_entries_in_the_central_directory, 2);
    memcpy(bytes + 12, &size_of_the_central_directory, 4);
    memcpy(bytes + 16, &offset_of_start_of_central_directory, 4);
    memcpy(bytes + 20, &zip_file_comment_length, 2);
    memcpy(bytes + 22, zip_file_comment, zip_file_comment_length);

    return bytes;
}
