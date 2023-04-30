#include "File.h"

#include <exception>
#include <fstream>
#include <tuple>

File::File(const LFH& lfh, char* compressed_data) : compressed_data(compressed_data), lfh(lfh), size_with_header(lfh.byte_size + lfh.compressed_size)
{
}

File::~File()
{
    delete[] compressed_data;
}

File::File(const char* path) : lfh(ZipFile::Fields::version_needed_to_extract::v1_0, ZipFile::Fields::general_purpose_bit_flag::None)
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
    
    lfh = LFH(static_cast<ZipFile::Fields::version_needed_to_extract>(ZipFile::current_version),
              ZipFile::Fields::general_purpose_bit_flag::None,
              ZipFile::Fields::compression_method::Stored,
              0, 0, 0, std::get<1>(compressed_data_and_size), file_size);

    compressed_data = std::get<0>(compressed_data_and_size);

    size_with_header = lfh.byte_size + lfh.compressed_size;
    
    delete[] data;
}

char* File::to_bytes() const
{
    char* bytes = new char[size_with_header];

    memcpy(bytes, lfh.to_bytes(), lfh.byte_size);
    memcpy(bytes + lfh.byte_size, compressed_data, lfh.compressed_size);

    return bytes;
}

const std::tuple<char*, int> File::get_compressed_data(char* data, int file_size) const
{
    char* compressed_data = new char[file_size];
    switch (lfh.compression_method)
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
    switch (lfh.compression_method) {
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

