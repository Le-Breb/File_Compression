#include "ZipFile.h"

#include <fstream>
#include <iostream>

#include "EOCD.h"

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