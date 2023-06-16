
#include <bitset>
#include <fstream>
#include <iostream>
#include <ostream>
#include <chrono>

#include "ZipFile.h"
#include "Compressors/Deflate/Main.h"
#include "LFH.h"


void show_file_content(const char* path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open()) throw std::runtime_error("unable to open file");
    in.seekg(0, std::ios::end);
    const size_t size = in.tellg();
    in.seekg(0);
    char* data = new char[size];
    in.read(data, static_cast<int>(size));
    for (int i = 0; i < size; i++)
        std::cout << (Byte) *(data + i);
    std::cout << std::endl;
}

void show_file_content2(const char* path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!in.is_open()) throw std::runtime_error("unable to open file");
    const size_t size = in.tellg();
    in.seekg(0);
    char* data = new char[size];
    in.read(data, static_cast<int>(size));
    int i = 0;
    i += LFH::Display_LFH((Byte*) data, i);
    std::cout << std::endl;
}

template<std::size_t N>
std::bitset<N> reverse(const std::bitset<N>& bit_set)
{
    std::bitset<N> reversed;
    for (int i = 0, j = N - 1; i < N; i++, j--)
    {
        reversed[j] = bit_set[i];
    }
    return reversed;
}

//ToDo: Writing bytes directly to file instead of returning them as a char* and then writing them to file
//ToDo: Procedurally check all dates and times
//ToDo: Add support for directories
//Todo: Allocate memory once and then reuse it
int main(int argc, char* argv[])
{
    //Deflate::Main::Test_file(
    //      R"(C:\Users\matmu\Documents\Unity\Projects\project-s2\Museum Leap\Library\ArtifactDB)", true);
    //return 0;
    //Deflate::Main::Test();
    ZipFile zip_file;
    zip_file.add_folder("../../FileCompression", "FileCompression");
    zip_file.write("../Data/someData_.zip");
    //show_file_content("../Data/someData_.zip");
    std::cout << std::flush;
    //show_file_content("../Data/someData.zip");
    zip_file.list_files();
    ZipFile zip_file2("../Data/someData.zip");
    zip_file2.list_files();

    return 0;
}



// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT - https://www.ietf.org/rfc/rfc1951.txt
