
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

//ToDo: Writing and reading bytes directly from an to file instead of storing them in a char array
//ToDo: Procedurally check all dates and times
//Todo: Reorder compute_dynamic_trees
//Todo: Tester le cas où il y a plus d138 0s à la suite dans enumerate_code_lengths
//Todo: Store single literals as a match of length and dist 0, so that we can make a symbol buffer of fixed size
int main(int argc, char* argv[])
{
    Deflate::Main::Test_file(
            R"(C:\Users\matmu\Documents\Unity\Projects\project-s2\MuseumLeapInstaller.exe)", true);

    Deflate::Main::Test();
    return 0;
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
