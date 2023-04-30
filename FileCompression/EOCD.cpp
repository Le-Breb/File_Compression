#include <stdexcept>
#include "EOCD.h"

#include <iostream>

#include "Exceptions.h"

EOCD::EOCD(short number_of_this_disk,
    short number_of_disk_with_start_of_central_directory,
    short total_number_of_entries_in_the_central_directory_on_this_disk,
    short total_number_of_entries_in_the_central_directory, int size_of_the_central_directory,
    int offset_of_start_of_central_directory, short zip_file_comment_length, const char* zip_file_comment) :
    number_of_this_disk(number_of_this_disk),
    number_of_disk_with_start_of_central_directory(number_of_disk_with_start_of_central_directory),
    total_number_of_entries_in_the_central_directory_on_this_disk(
        total_number_of_entries_in_the_central_directory_on_this_disk),
    total_number_of_entries_in_the_central_directory(total_number_of_entries_in_the_central_directory),
    size_of_the_central_directory(size_of_the_central_directory),
    offset_of_start_of_central_directory(offset_of_start_of_central_directory),
    zip_file_comment_length(zip_file_comment_length), zip_file_comment(zip_file_comment), byte_size(22 + zip_file_comment_length)
{
}

EOCD::~EOCD()
{
    delete[] zip_file_comment;
}

char* EOCD::to_bytes() const
{
    //
    char* bytes = new char[byte_size];
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
