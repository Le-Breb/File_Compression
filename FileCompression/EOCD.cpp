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

EOCD* EOCD::parse(const char* bytes, const int& offset)
{
    bytes += offset;

    const short number_of_this_disk = *(bytes + 4);
    const short number_of_disk_with_start_of_central_directory = *(bytes + 6);
    const short total_number_of_entries_in_the_central_directory_on_this_disk = *(bytes + 8);
    const short total_number_of_entries_in_the_central_directory = *(bytes + 10);
    const int size_of_the_central_directory = *(bytes + 12);
    const int offset_of_start_of_central_directory = *(bytes + 16);
    const short zip_file_comment_length = *(bytes + 20);
    const char* zip_file_comment = zip_file_comment_length > 0 ? bytes + 22 : nullptr;

    EOCD* eocd = new EOCD(number_of_this_disk, number_of_disk_with_start_of_central_directory,
        total_number_of_entries_in_the_central_directory_on_this_disk,
        total_number_of_entries_in_the_central_directory, size_of_the_central_directory,
        offset_of_start_of_central_directory, zip_file_comment_length, zip_file_comment);

    return eocd;
}

EOCD::~EOCD()
{
    delete[] zip_file_comment;
}

char* EOCD::to_bytes() const
{
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

    std::cout << std::hex << signature << " ";
    std::cout << (number_of_this_disk << 2 | number_of_disk_with_start_of_central_directory) << " ";
    std::cout << (total_number_of_entries_in_the_central_directory_on_this_disk << 2 | total_number_of_entries_in_the_central_directory) << " ";
    std::cout << size_of_the_central_directory << " " << offset_of_start_of_central_directory << " " << zip_file_comment_length << " comment" << std::endl;
    
    return bytes;
}

std::ostream& operator<<(std::ostream& os, const EOCD& eocd)
{
    os <<std::dec<< "EOCD: " << "number_of_this_disk: " << eocd.number_of_this_disk << ", number_of_disk_with_start_of_central_directory: " << eocd.number_of_disk_with_start_of_central_directory << ", total_number_of_entries_in_the_central_directory_on_this_disk: " << eocd.total_number_of_entries_in_the_central_directory_on_this_disk << ", total_number_of_entries_in_the_central_directory: " << eocd.total_number_of_entries_in_the_central_directory << ", size_of_the_central_directory: " << eocd.size_of_the_central_directory << ", offset_of_start_of_central_directory: " << eocd.offset_of_start_of_central_directory << ", zip_file_comment_length: " << eocd.zip_file_comment_length << ", zip_file_comment: " << eocd.zip_file_comment;
    return os;
}
