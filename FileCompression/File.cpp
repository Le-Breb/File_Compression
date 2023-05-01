#include "File.h"

#include <exception>
#include <fstream>
#include <tuple>

File::~File()
{
    delete[] compressed_data;
}

File::File(const char* path, ZipFile::Fields::compression_method compression_method)
{
    std::ifstream in(path);
    if (!in.is_open()) throw std::exception("File not found");
    int file_size = 0;
    in.seekg(0, std::ios::end);
    file_size = in.tellg();
    char* data = new char[file_size];
    in.seekg(0, std::ios::beg);
    in.read(data, file_size);
    in.close();

    const auto compressed_data_and_size = get_compressed_data(data, file_size);

    version_needed_to_extract = static_cast<ZipFile::Fields::version_needed_to_extract>(ZipFile::current_version);
    general_purpose_bit_flag = ZipFile::Fields::general_purpose_bit_flag::None;
    this->compression_method = compression_method;
    last_mod_file_time = 0;
    last_mod_file_date = 0;
    crc32 = 0;
    compressed_size = std::get<1>(compressed_data_and_size);
    uncompressed_size = file_size;
    file_name_length = strlen(path);
    extra_field_length = 0;
    file_comment_length = 0;
    disk_number_start = 0;
    internal_file_attributes = 0;
    external_file_attributes = 0;
    file_name = path;
    extra_field = nullptr;
    file_comment = nullptr;

    compressed_data = std::get<0>(compressed_data_and_size);
    
    delete[] data;
}

std::tuple<char*, int> File::get_compressed_data(char* data, int file_size) const
{
    char* compressed_data = new char[file_size];
    switch (compression_method)
    {
        case ZipFile::Fields::compression_method::Stored:
            memcpy(compressed_data, data, file_size);
            return std::tuple<char*, int>(compressed_data, file_size);
        case ZipFile::Fields::compression_method::Shrunk: break;
        case ZipFile::Fields::compression_method::Reduced_1: break;
        case ZipFile::Fields::compression_method::Reduced_2: break;
        case ZipFile::Fields::compression_method::Reduced_3: break;
        case ZipFile::Fields::compression_method::Reduced_4: break;
        case ZipFile::Fields::compression_method::Imploded: break;
        case ZipFile::Fields::compression_method::Reserved_1: break;
        case ZipFile::Fields::compression_method::Deflated: break;
        case ZipFile::Fields::compression_method::Enhanced_Deflated: break;
        case ZipFile::Fields::compression_method::PKWare_DCL_Implode: break;
        case ZipFile::Fields::compression_method::Reserved_2: break;
        case ZipFile::Fields::compression_method::BZIP2: break;
        case ZipFile::Fields::compression_method::Reserved_3: break;
        case ZipFile::Fields::compression_method::LZMA: break;
        case ZipFile::Fields::compression_method::Reserved_4: break;
        case ZipFile::Fields::compression_method::Reserved_5: break;
        case ZipFile::Fields::compression_method::Reserved_6: break;
        case ZipFile::Fields::compression_method::IBM_TERSE: break;
        case ZipFile::Fields::compression_method::IBM_LZ77_z: break;
        case ZipFile::Fields::compression_method::MP3: break;
        case ZipFile::Fields::compression_method::XZ: break;
        case ZipFile::Fields::compression_method::JPEG: break;
        case ZipFile::Fields::compression_method::WavPack: break;
        case ZipFile::Fields::compression_method::PPMD: break;
        case ZipFile::Fields::compression_method::AE_x: break;
        case ZipFile::Fields::compression_method::Unknown: break;
        default: throw std::exception("Unknown compression method");
    }

    throw std::exception("Not implemented");
}

const std::tuple<char*, int> File::get_uncompressed_data(char* data, int compressed_file_size) const
{
    switch (compression_method) {
        case ZipFile::Fields::compression_method::Stored: break;
        case ZipFile::Fields::compression_method::Shrunk: break;
        case ZipFile::Fields::compression_method::Reduced_1: break;
        case ZipFile::Fields::compression_method::Reduced_2: break;
        case ZipFile::Fields::compression_method::Reduced_3: break;
        case ZipFile::Fields::compression_method::Reduced_4: break;
        case ZipFile::Fields::compression_method::Imploded: break;
        case ZipFile::Fields::compression_method::Reserved_1: break;
        case ZipFile::Fields::compression_method::Deflated: break;
        case ZipFile::Fields::compression_method::Enhanced_Deflated: break;
        case ZipFile::Fields::compression_method::PKWare_DCL_Implode: break;
        case ZipFile::Fields::compression_method::Reserved_2: break;
        case ZipFile::Fields::compression_method::BZIP2: break;
        case ZipFile::Fields::compression_method::Reserved_3: break;
        case ZipFile::Fields::compression_method::LZMA: break;
        case ZipFile::Fields::compression_method::Reserved_4: break;
        case ZipFile::Fields::compression_method::Reserved_5: break;
        case ZipFile::Fields::compression_method::Reserved_6: break;
        case ZipFile::Fields::compression_method::IBM_TERSE: break;
        case ZipFile::Fields::compression_method::IBM_LZ77_z: break;
        case ZipFile::Fields::compression_method::MP3: break;
        case ZipFile::Fields::compression_method::XZ: break;
        case ZipFile::Fields::compression_method::JPEG: break;
        case ZipFile::Fields::compression_method::WavPack: break;
        case ZipFile::Fields::compression_method::PPMD: break;
        case ZipFile::Fields::compression_method::AE_x: break;
        case ZipFile::Fields::compression_method::Unknown: break;
        default: ;
    }

    throw new std::exception("Not implemented");
}

