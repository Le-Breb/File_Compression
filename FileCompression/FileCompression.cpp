
#include <bitset>
#include <fstream>
#include <iostream>
#include <ostream>

#include "CRC32.h"
#include "ZipFile.h"
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
        std::cout << std::hex << *(data + i);
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    /*std::map<char, int> t;
    t['a'] = 4;
    t['b'] = 4;
    t['c'] = 2;
    t['e'] = 1;
    t['t'] = 5;
    Huffman_Tree huffman_tree(t);*/
    ZipFile zip_file;
    zip_file.add_file("someData.txt");
    zip_file.write("someData_.zip");
    show_file_content("someData_.zip");
    show_file_content("someData.zip");
    ZipFile zip_file2("someData.zip");
    
    return 0;
}



// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT - https://www.ietf.org/rfc/rfc1951.txt
//ToDo: Writing bytes directly to file instead of returning them as a char* and then writing them to file


// Reminder : characters are encoded using a canonical huffman tree. This tree is encoded by only writing the code
// lengths of the alphabet (which is the entire ASCII table). This is enough to be able to build the entire tree when
// decoding the file. But the code lengths are themselves encoded using a huffman code defined in the DEFLATE
// specifications.