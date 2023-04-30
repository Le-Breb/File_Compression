#include "ZipFile.h"

#include <fstream>
#include <iostream>
#include <map>

#include "EOCD.h"
#include "File.h"
#include "CDFH.h"

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
    std::cout << eocd.byte_size << std::endl; 
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
        outfile.write(file->to_bytes(), file->size_with_header);
        file_offsets[file] = offset;
        offset += file->size_with_header;
    }
    const int cd_start_offset = offset;

    // Write CD
    for (auto file : files)
    {
        CDFH cdfh = CDFH(file->lfh);
        cdfh.relative_offset_of_local_header = offset - file_offsets[file];
        outfile.write(cdfh.to_bytes(), cdfh.byte_size);
        offset += cdfh.byte_size;
    }

    const int cd_size = offset - cd_start_offset;

    // Write EOCD
    const int num_files = files.size();
    EOCD eocd = EOCD(disk_number, disk_number, num_files, num_files, cd_size, cd_start_offset, 0);
    outfile.write(eocd.to_bytes(), eocd.byte_size);
}
