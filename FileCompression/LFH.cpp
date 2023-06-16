#include "LFH.h"

#include <cstring>
#include <iostream>

#include "File.h"

LFH::LFH(ZipFile::Fields::version_needed_to_extract version_needed_to_extract,
         ZipFile::Fields::general_purpose_bit_flag general_purpose_bit_flag,
         ZipFile::Fields::compression_method compression_method, const MS_DOS::Time& last_mod_file_time,
         const MS_DOS::Date& last_mod_file_date,
         unsigned int crc32, unsigned int compressed_size, unsigned int uncompressed_size,
         unsigned short file_name_length, unsigned short extra_field_length,
         char* file_name, char* extra_field) : version_needed_to_extract(version_needed_to_extract),
                                               general_purpose_bit_flag(general_purpose_bit_flag),
                                               compression_method(compression_method),
                                               last_mod_file_time(new MS_DOS::Time(last_mod_file_time)),
                                               last_mod_file_date(new MS_DOS::Date(last_mod_file_date)), crc32(crc32),
                                               compressed_size(compressed_size),
                                               uncompressed_size(uncompressed_size),
                                               file_name_length(file_name_length),
                                               extra_field_length(extra_field_length), file_name(file_name),
                                               extra_field(extra_field),
                                               byte_size(30 + file_name_length + extra_field_length)
{
}

LFH::LFH(std::ifstream& in, const unsigned offset)
{
    in.seekg(offset);
    int signature_check;
    in.read(reinterpret_cast<char*>(&signature_check), 4);
    if (signature_check != signature)
        throw std::runtime_error("Invalid signature");
    in.read(reinterpret_cast<char*>(&version_needed_to_extract), 2);
    in.read(reinterpret_cast<char*>(&general_purpose_bit_flag), 2);
    in.read(reinterpret_cast<char*>(&compression_method), 2);
    in.read(reinterpret_cast<char*>(&last_mod_file_time), 2);
    in.read(reinterpret_cast<char*>(&last_mod_file_date), 2);
    in.read(reinterpret_cast<char*>(&crc32), 4);
    in.read(reinterpret_cast<char*>(&compressed_size), 4);
    in.read(reinterpret_cast<char*>(&uncompressed_size), 4);
    in.read(reinterpret_cast<char*>(&file_name_length), 2);
    in.read(reinterpret_cast<char*>(&extra_field_length), 2);
    file_name = new char[file_name_length + 1];
    in.read(file_name, file_name_length);
    file_name[file_name_length] = '\0';
    extra_field = new char[extra_field_length + 1];
    in.read(extra_field, extra_field_length);
    extra_field[extra_field_length] = '\0';
    byte_size = 30 + file_name_length + extra_field_length;
}

LFH::~LFH()
{
    //delete[] file_name;
    //delete[] extra_field;
    //delete last_mod_file_time;
    //delete last_mod_file_date;
}

std::pair<char*, int> LFH::build_from(const File& file)
{
    int byte_size = 30 + file.file_name_length + file.extra_field_length;
    char* bytes = new char[byte_size];
    memcpy(bytes, &signature, 4);
    memcpy(bytes + 4, &file.version_needed_to_extract, 2);
    memcpy(bytes + 6, &file.general_purpose_bit_flag, 2);
    memcpy(bytes + 8, &file.compression_method, 2);
    memcpy(bytes + 10, &file.last_mod_file_time->bytes, 2);
    memcpy(bytes + 12, &file.last_mod_file_date->bytes_, 2);
    memcpy(bytes + 14, &file.crc32, 4);
    memcpy(bytes + 18, &file.compressed_size, 4);
    memcpy(bytes + 22, &file.uncompressed_size, 4);
    memcpy(bytes + 26, &file.file_name_length, 2);
    memcpy(bytes + 28, &file.extra_field_length, 2);
    memcpy(bytes + 30, file.file_name, file.file_name_length);
    memcpy(bytes + 30 + file.file_name_length, file.extra_field, file.extra_field_length);

    return {bytes, byte_size};
}

char* LFH::to_bytes() const
{
    char* bytes = new char[byte_size];

    memcpy(bytes, &signature, 4);
    memcpy(bytes + 4, &version_needed_to_extract, 2);
    memcpy(bytes + 6, &general_purpose_bit_flag, 2);
    memcpy(bytes + 8, &compression_method, 2);
    memcpy(bytes + 10, &last_mod_file_time, 2);
    memcpy(bytes + 12, &last_mod_file_date, 2);
    memcpy(bytes + 14, &crc32, 4);
    memcpy(bytes + 18, &compressed_size, 4);
    memcpy(bytes + 22, &uncompressed_size, 4);
    memcpy(bytes + 26, &file_name_length, 2);
    memcpy(bytes + 28, &extra_field_length, 2);
    memcpy(bytes + 30, file_name, file_name_length);
    memcpy(bytes + 30 + file_name_length, extra_field, extra_field_length);

    return bytes;
}

int LFH::Display_LFH(const Byte* data, int offset)
{
    std::cout << "Local File Header" << std::endl;
    std::cout << "Signature: " << std::hex << data[offset] << data[offset + 1] << data[offset + 2] << data[offset + 3]
              << std::endl;
    std::cout << "Version needed to extract: " << std::hex << data[offset + 4] << data[offset + 5] << std::endl;
    std::cout << "General purpose bit flag: " << std::hex << data[offset + 6] << data[offset + 7] << std::endl;
    std::cout << "Compression method: " << std::hex << data[offset + 8] << data[offset + 9] << std::endl;
    std::cout << "Last mod file time: " << std::hex << data[offset + 10] << data[offset + 11] << std::endl;
    std::cout << "Last mod file date: " << std::hex << data[offset + 12] << data[offset + 13] << std::endl;
    long crc32 = data[offset + 14] + (data[offset + 15] << 8) + (data[offset + 16] << 16) + (data[offset + 17] << 24);
    std::cout << "CRC-32: " << std::dec << crc32 << std::endl;
    int compressed_size = data[offset + 18] + (data[offset + 19] << 8) + (data[offset + 20] << 16) +
                          (data[offset + 21] << 24);
    std::cout << "Compressed size: " << std::dec << compressed_size << std::endl;
    int uncompressed_size = data[offset + 22] + (data[offset + 23] << 8) + (data[offset + 24] << 16) +
                            (data[offset + 25] << 24);
    std::cout << "Uncompressed size: " << std::dec << uncompressed_size << std::endl;
    int file_name_length = data[offset + 26] + (data[offset + 27] << 8);
    std::cout << "File name length: " << std::dec << file_name_length << std::endl;
    int extra_field_length = data[offset + 28] + (data[offset + 29] << 8);
    std::cout << "Extra field length: " << std::dec << extra_field_length << std::endl;
    std::cout << "File name: ";
    for (int i = 0; i < data[offset + 26]; i++)
    {
        std::cout << data[offset + 30 + i];
    }
    std::cout << std::endl;
    std::cout << "Extra field: ";
    for (int i = 0; i < data[offset + 28]; i++)
    {
        std::cout << data[offset + 30 + data[offset + 26] + i];
    }
    std::cout << std::endl;

    return offset + 30 + data[offset + 26] + data[offset + 28];
}

