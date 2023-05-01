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
    int index = file_size; // index of the last byte + 1
    int signature_search = 0;
    while (index >= 0 && signature_search != EOCD::signature)
    {
        in.seekg(-5 * sizeof(char), std::ios::cur);
        index--;
        in.read(reinterpret_cast<char*>(&signature_search), 4);
    }

    if (signature_search != EOCD::signature) throw invalid_file(invalid_file::Reason::NO_EOCD);
    const int eocd_size = file_size - index;
    char* eocd_bytes = new char[eocd_size];
    in.read(eocd_bytes, eocd_size);
    
    return EOCD::parse(eocd_bytes, -4);
}

ZipFile::~ZipFile()
{
    for (const auto file : files)
    {
        delete file;
    }
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
    File* file = new File(filename);
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
    }
    const int cd_start_offset = offset;

    // Write CD
    for (auto file : files)
    {
        const int relative_offset_of_local_header = offset - file_offsets[file];
        std::tuple<char*, int> cdfh_bytes_and_size = CDFH::build_from(*file, relative_offset_of_local_header);
        outfile.write(std::get<0>(cdfh_bytes_and_size), std::get<1>(cdfh_bytes_and_size));
        outfile.write(file->compressed_data, file->compressed_size);
        offset += std::get<1>(cdfh_bytes_and_size);
    }

    const int cd_size = offset - cd_start_offset;

    // Write EOCD
    const int num_files = files.size();
    EOCD eocd = EOCD(disk_number, disk_number, num_files, num_files, cd_size, cd_start_offset, 0);
    outfile.write(eocd.to_bytes(), eocd.byte_size);
}

ZipFile::ZipFile()
= default;

ZipFile::ZipFile(const char* filename)
{
    std::ifstream in(filename, std::ios::binary | std::ios::in);
    if (!in.is_open()) throw std::exception("File not found");

    EOCD* eocd = find_eocd(in);

    std::cout << *eocd << std::endl;
    
    delete eocd;
}
