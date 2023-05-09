
#include <fstream>
#include <iostream>
#include <ostream>

#include "CRC32.h"
#include "ZipFile.h"
#include "Compressors/Huffman_Tree/Huffman_Tree.h"

int main(int argc, char* argv[])
{
    std::map<char, int> t;
    t['a'] = 4;
    t['b'] = 4;
    t['c'] = 2;
    t['e'] = 1;
    t['t'] = 5;
    Huffman_Tree huffman_tree(t);
    /*CRC32 crc32;
    std::cout << std::hex << crc32.compute(new char[2] {73,73}, 2) << std::endl;
    return 0;*/
    //ZipFile zip_file;
    //zip_file.add_file("someData.txt");
    //zip_file.write("someData.zip");
    //ZipFile zip_file2("someData.zip");
    
    return 0;
}

// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
//ToDo: Writing bytes directly to file instead of returning them as a char* and then writing them to file
