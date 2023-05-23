
#include <bitset>
#include <fstream>
#include <iostream>
#include <ostream>

#include "CRC32.h"
#include "ZipFile.h"
#include "Compressors/Deflate.h"
#include "Compressors/Huffman_Tree/Huffman_Tree.h"
void show_file_content(const char* path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open()) throw std::runtime_error("unable to open file");
    in.seekg(0, std::ios::end);
    const size_t size = in.tellg();
    in.seekg(0);
    char* data = new char[size];
    in.read(data, size);
    for (int i = 0; i < size; i++)
        std::cout <<(unsigned char)*(data + i);
    std::cout << std::endl;
}

template<std::size_t N>
std::bitset<N> reverse(const std::bitset<N> &bit_set) {
    std::bitset<N> reversed;
    for (int i = 0, j = N - 1; i < N; i++, j--) {
        reversed[j] = bit_set[i];
    }
    return reversed;
}

int main(int argc, char* argv[])
{
    /*const unsigned char data[] = {0x1d, 0xc6, 0x49, 0x01, 0x00, 0x00, 0x10, 0x40, 0xc0, 0xac, 0xa3, 0x7f, 0x88, 0x3d, 0x3c, 0x20, 0x2a, 0x97, 0x9d, 0x37, 0x5e, 0x1d, 0x0c};
    auto a = Deflate::inflate(data, 0);
    for (int i = 0; i < a.second; ++i) {
        std::cout<<a.first[i];
    }
    std::cout << std::endl;*/
    ZipFile zip_file;
    zip_file.add_file("../Data/someData.txt", "someData.txt");
    zip_file.write("../Data/someData_.zip");
    show_file_content("../Data/someData_.zip");
    show_file_content("../Data/someData.zip");
    ZipFile zip_file2("../Data/someData.zip");

    return 0;
}//Todo: make window cross blocks



// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT - https://www.ietf.org/rfc/rfc1951.txt
//ToDo: Writing bytes directly to file instead of returning them as a char* and then writing them to file
//ToDo: Procedurally check all dates and times


// Reminder : characters are encoded using a canonical huffman tree. This tree is encoded by only writing the code
// lengths of the alphabet (which is the entire ASCII table). This is enough to be able to build the entire tree when
// decoding the file. But the code lengths are themselves encoded using a huffman code defined in the DEFLATE
// specifications.