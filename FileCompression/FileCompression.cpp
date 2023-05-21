
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
    if (!in.is_open()) throw std::exception("unable to open file");
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

    const auto data = new unsigned char[] {0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x57, 0xc8, 0x40, 0x27, 0xb9, 0x00};
    auto decompressed_data = Deflate::decompress(data, 0);
    auto a = std::get<0>(decompressed_data);
    for (int i = 0; i < std::get<1>(decompressed_data); i++)
        std::cout << std::hex << (unsigned char)std::get<0>(decompressed_data)[i];
    return 0;
    /*std::map<char, int> t;
    t['a'] = 4;
    t['b'] = 4;
    t['c'] = 2;
    t['e'] = 1;
    t['t'] = 5;
    Huffman_Tree huffman_tree(t);*/
    std::cout<< std::hex << 43 << ' ' << 46 << ' ' << 134 << ' ' << 1 << ' ' << 0 << std::endl;
    std::cout << std::bitset<8>(43) << ' ' << std::bitset<8>(46) << ' ' << std::bitset<8>(134) << ' ' << std::bitset<8>(1) << ' ' << std::bitset<8>(0) << std::endl;
    std::cout << reverse(std::bitset<8>(43)) << ' ' << reverse(std::bitset<8>(46)) << ' ' << reverse(std::bitset<8>(134)) << ' ' << reverse(std::bitset<8>(1)) << ' ' << reverse(std::bitset<8>(0)) << std::endl;
    ZipFile zip_file;
    zip_file.add_file("someData - Copie.txt");
    zip_file.write("someData_ - Copie.zip");
    show_file_content("someData_ - Copie.zip");
    show_file_content("someData - Copie.zip");
    ZipFile zip_file2("someData_ - Copie.zip");
    
    return 0;
}//Todo: make window cross blocks



// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT - https://www.ietf.org/rfc/rfc1951.txt
//ToDo: Writing bytes directly to file instead of returning them as a char* and then writing them to file


// Reminder : characters are encoded using a canonical huffman tree. This tree is encoded by only writing the code
// lengths of the alphabet (which is the entire ASCII table). This is enough to be able to build the entire tree when
// decoding the file. But the code lengths are themselves encoded using a huffman code defined in the DEFLATE
// specifications.