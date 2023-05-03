#include <stdexcept>
#include "EOCD.h"

#include <iostream>

#include "Exceptions.h"

EOCD::EOCD(unsigned short number_of_this_disk,
    unsigned short number_of_disk_with_start_of_central_directory,
    unsigned short total_number_of_entries_in_the_central_directory_on_this_disk,
    unsigned short total_number_of_entries_in_the_central_directory, unsigned int size_of_the_central_directory,
    unsigned int offset_of_start_of_central_directory, unsigned short zip_file_comment_length, char* zip_file_comment) :
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

EOCD::EOCD(std::ifstream& file, unsigned offset)
{
    file.seekg(offset);
    unsigned int signature_check = 0;
    file.read(reinterpret_cast<char*>(&signature_check), 4);
    if (signature_check != signature)
        throw invalid_file(invalid_file::Reason::INVALID_EOCD);
    file.read(reinterpret_cast<char*>(&number_of_this_disk), 2);
    file.read(reinterpret_cast<char*>(&number_of_disk_with_start_of_central_directory), 2);
    file.read(reinterpret_cast<char*>(&total_number_of_entries_in_the_central_directory_on_this_disk), 2);
    file.read(reinterpret_cast<char*>(&total_number_of_entries_in_the_central_directory), 2);
    file.read(reinterpret_cast<char*>(&size_of_the_central_directory), 4);
    file.read(reinterpret_cast<char*>(&offset_of_start_of_central_directory), 4);
    file.read(reinterpret_cast<char*>(&zip_file_comment_length), 2);
    zip_file_comment = new char[zip_file_comment_length + 1];
    file.read(zip_file_comment, zip_file_comment_length);
    zip_file_comment[zip_file_comment_length] = '\0';
    byte_size = 22 + zip_file_comment_length;
    file.clear(); // Clear EOF flag to allow continuing reading (otherwise, the next read will fail)
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
    os << std::dec << "EOCD: " << "number_of_this_disk: " << eocd.number_of_this_disk <<
        ", number_of_disk_with_start_of_central_directory: " << eocd.number_of_disk_with_start_of_central_directory <<
        ", total_number_of_entries_in_the_central_directory_on_this_disk: " << eocd.
        total_number_of_entries_in_the_central_directory_on_this_disk <<
        ", total_number_of_entries_in_the_central_directory: " << eocd.total_number_of_entries_in_the_central_directory
        << ", size_of_the_central_directory: " << eocd.size_of_the_central_directory <<
        ", offset_of_start_of_central_directory: " << eocd.offset_of_start_of_central_directory <<
        ", zip_file_comment_length: " << eocd.zip_file_comment_length;
    if (eocd.zip_file_comment_length > 0)
        os << ", zip_file_comment: " << eocd.zip_file_comment;
    return os;
}
