#include "ZipFile.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>

#include "EOCD.h"
#include "File.h"
#include "CDFH.h"
#include "Exceptions.h"

EOCD* ZipFile::find_eocd(std::ifstream& in)
{
    in.seekg(sizeof(char), std::ios::end);
    const int file_size = in.tellg();
    int offset = file_size; // index of the last byte + 1
    int signature_search = 0;
    while (offset >= 0 && signature_search != EOCD::signature)
    {
        in.seekg(-5 * sizeof(char), std::ios::cur);
        offset--;
        in.read(reinterpret_cast<char*>(&signature_search), 4);
    }

    if (signature_search != EOCD::signature) throw invalid_file(invalid_file::Reason::NO_EOCD);
    
    return new EOCD(in, offset - 4);
}

void ZipFile::register_files(std::ifstream& in, const int& offset_of_start_of_central_directory, const int& central_directory_size)
{
    int offset = offset_of_start_of_central_directory;
    while (offset < offset_of_start_of_central_directory + central_directory_size)
    {
        CDFH cdfh(in, offset);
        File* file = new File(in, cdfh);
        files.push_back(file);
        offset += cdfh.byte_size;
    }
}

void ZipFile::list_files() const
{
    std::cout << "Files: (" << files.size() << ")" << std::endl;
    for (const auto file : files)
    {
        std::cout << *file << std::endl;
    }
}

MS_DOS::Date ZipFile::get_date_from_system()
{
    
    time_t now = time(0);
#pragma warning(disable:4996)
    tm* ltm = localtime(&now);
#pragma warning(default:4996)
    auto b = localtime_s(ltm, &now);
    
    return MS_DOS::Date(ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday);
}

MS_DOS::Time ZipFile::get_time_from_system()
{
    time_t now = time(0);
#pragma warning(disable:4996)
    tm* ltm = localtime(&now);
#pragma warning(default:4996)
    return MS_DOS::Time(ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
}

ZipFile::~ZipFile()
{
    for (const auto file : files)
    {
        delete file;
    }
    delete creation_date_;
    delete creation_time_;
}

void ZipFile::write_empty_zip_file(const char* filename)
{
    std::ofstream outfile;
    outfile.open(filename, std::ios::binary | std::ios::out);
    EOCD eocd = EOCD();
    char* eocd_bytes = eocd.to_bytes();
    outfile.write(eocd_bytes, eocd.byte_size);
    outfile.close();
}

void ZipFile::add_file(const char* filename)
{
    File* file = new File(filename, Fields::compression_method::Stored, *creation_time_, *creation_date_);
    files.push_back(file);
}

void ZipFile::write(const char* filename)
{
    std::ofstream outfile;
    outfile.open(filename, std::ios::binary | std::ios::out);
    std::map<File*, int> file_offsets;
    int offset = 0;

    // Write all files
    for (auto file : files)
    {
        std::tuple<char*, int> lfh_bytes_and_size = LFH::build_from(*file);
        outfile.write(std::get<0>(lfh_bytes_and_size), std::get<1>(lfh_bytes_and_size));
        outfile.write(file->compressed_data, file->compressed_size);
        file_offsets[file] = offset;
        offset += std::get<1>(lfh_bytes_and_size) + file->compressed_size;
        delete std::get<0>(lfh_bytes_and_size);
    }
    const int cd_start_offset = offset;

    // Write all CDFHs
    for (auto file : files)
    {
        const int relative_offset_of_local_header = file_offsets[file];
        std::tuple<char*, int> cdfh_bytes_and_size = CDFH::build_from(*file, relative_offset_of_local_header);
        outfile.write(std::get<0>(cdfh_bytes_and_size), std::get<1>(cdfh_bytes_and_size));
        outfile.write(file->compressed_data, file->compressed_size);
        offset += std::get<1>(cdfh_bytes_and_size);
        delete std::get<0>(cdfh_bytes_and_size);
    }

    const int cd_size = offset - cd_start_offset;

    // Write EOCD
    const int num_files = files.size();
    EOCD eocd = EOCD(disk_number, disk_number, num_files, num_files, cd_size, cd_start_offset, 0);
    outfile.write(eocd.to_bytes(), eocd.byte_size);
}

ZipFile::ZipFile() : creation_date_(new MS_DOS::Date(get_date_from_system())), creation_time_(new MS_DOS::Time(get_time_from_system()))
{
}

ZipFile::ZipFile(const char* filename) : creation_date_(new MS_DOS::Date(get_date_from_system())), creation_time_(new MS_DOS::Time(get_time_from_system()))
{
    std::ifstream in(filename, std::ios::binary | std::ios::in);
    if (!in.is_open()) throw std::exception("File not found");

    EOCD* eocd = find_eocd(in);
    std::cout << *eocd << std::endl;
    register_files(in, eocd->offset_of_start_of_central_directory, eocd->size_of_the_central_directory);
    list_files();
    
    delete eocd;
}

const std::map<ZipFile::Fields::general_purpose_bit_flag, std::string> ZipFile::Fields::general_purpose_bit_flag_to_string = {
            { general_purpose_bit_flag::Encrypted, "encrypted" },
            { general_purpose_bit_flag::Compression_option_1, "compression_option1" },
            { general_purpose_bit_flag::Compression_option_2, "compression_option2" },
            { general_purpose_bit_flag::Data_descriptor, "data_descriptor" },
            { general_purpose_bit_flag::Enhanced_deflation, "enhanced_deflating" },
            { general_purpose_bit_flag::Compressed_patched_data, "compressed_patched_data" },
            { general_purpose_bit_flag::Strong_encryption, "strong_encryption" },
            /*{ general_purpose_bit_flag::Unused_8, "unused_8" },
            { general_purpose_bit_flag::Unused_9, "unused_9" },
            { general_purpose_bit_flag::Unused_10, "unused_10" },
            { general_purpose_bit_flag::Unused_11, "unused_11" },*/
            { general_purpose_bit_flag::UTF_8, "UTF-8" },
            /*{ general_purpose_bit_flag::Language_encoding, "language_encoding" },
            { general_purpose_bit_flag::Reserved_14, "reserved_14" },*/
            { general_purpose_bit_flag::Mask_header_values, "mask_header_values" }
            /*{ general_purpose_bit_flag::Reserved_16, "reserved_16" },
            { general_purpose_bit_flag::Reserved_17, "reserved_17" },
            { general_purpose_bit_flag::Reserved_18, "reserved_18" },
            { general_purpose_bit_flag::Reserved_19, "reserved_19" },
            { general_purpose_bit_flag::Reserved_20, "reserved_20" },
            { general_purpose_bit_flag::Reserved_21, "reserved_21" },
            { general_purpose_bit_flag::Reserved_22, "reserved_22" },
            { general_purpose_bit_flag::Reserved_23, "reserved_23" },
            { general_purpose_bit_flag::Reserved_24, "reserved_24" },
            { general_purpose_bit_flag::Reserved_25, "reserved_25" },
            { general_purpose_bit_flag::Reserved_26, "reserved_26" },
            { general_purpose_bit_flag::Reserved_27, "reserved_27" },
            { general_purpose_bit_flag::Reserved_28, "reserved_28" },
            { general_purpose_bit_flag::Reserved_29, "reserved_29" },
            { general_purpose_bit_flag::Reserved_30, "reserved_30" },
            { general_purpose_bit_flag::Reserved_31, "reserved_31" },*/
        };