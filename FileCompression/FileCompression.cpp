
#include <bitset>
#include <fstream>
#include <iostream>
#include <ostream>
#include <chrono>

#include "ZipFile.h"
#include "Compressors/Deflate/Main.h"
#include "Compressors/Deflate/Huffman_Tree.h"


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
//Todo: Make DEFLATE create blocks with MAX_SYMBOLS_PER_BLOCK symbols
//Todo: Change keys_path tree constructor to copy the key_paths and then be able to delete i
//Todo: Change tree constructor with frequency table to exclude codes with frequency of 0
// Commenting the three delete[] allow to encode one block
//Todo: Allocate memory once and then reuse it
int main(int argc, char* argv[])
{
    /*Byte* data = new Byte[13];
    const char* a = "hello hello!";
    for (int i = 0; i < 13; ++i)
        data[i] = a[i];
    std::vector<Byte> compressed_data = Deflate::Main::deflate(data, 13);*/
    /*for (int i = 1054000; i < 2147483647; i += 1000)//1054000
    {
        Byte* data = new Byte[i];
        Deflate::Main::deflate(data, i);
        std::cout << i << std::endl;
        delete[] data; // There is an access violation error somewhere in the code...
    }*/
    //return 0;
    Deflate::Main::Test();
    /*ZipFile zip_file;
    zip_file.add_file("../Data/someData.txt", "someData.txt");
    zip_file.write("../Data/someData_.zip");
    show_file_content("../Data/someData_.zip");
    show_file_content("../Data/someData.zip");
    ZipFile zip_file2("../Data/someData.zip");*/

    return 0;
}



// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT - https://www.ietf.org/rfc/rfc1951.txt
