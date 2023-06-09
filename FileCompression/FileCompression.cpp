
#include <bitset>
#include <fstream>
#include <iostream>
#include <ostream>

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
        std::cout << (unsigned char) *(data + i);
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

int main(int argc, char* argv[])
{
    /*char* path = "dict.txt";
    std::ofstream out(path, std::ios::out);
    for (int i = 3; i < 259; ++i) {
        int closestAbs = 99;
        int closest = 0;
        for(const auto& a : Deflate::Main::length_codes2) {
            if (a.first > i)
                continue;
            if (a.first == i) {
                closest = a.first;
                break;
            } else if (abs(a.first - i) < closestAbs) {
                closestAbs = abs(a.first - i);
                closest = a.first;
            }
        }
        out.write(", {", 3);
        out.write(std::to_string(i).c_str(), std::to_string(i).size());
        out.write(", ", 2);
        out.write(std::to_string(closest).c_str(), std::to_string(closest).size());
        out.write("}", 1);
    }
    return 0;*/
    /*auto a = "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";
    auto b = Base64::decode(a);
    auto c = new unsigned char[b.size()];
    for (int i = 0; i < b.size(); ++i)
        c[i] = b[i];
    auto d = Deflate::Main::inflate(c);
    for (int i = 0; i < d.second; ++i)
        std::cout << d.first[i];
    return 0;*/
    Deflate::Main::Test();
    return 0;
    /*ZipFile zip_file;
    zip_file.add_file("../Data/someData.txt", "someData.txt");
    zip_file.write("../Data/someData_.zip");
    show_file_content("../Data/someData_.zip");*/
    show_file_content("../Data/someData.zip");
    ZipFile zip_file2("../Data/someData.zip");

    return 0;
}//Todo: make window cross blocks



// Doc https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT - https://www.ietf.org/rfc/rfc1951.txt
//ToDo: Writing bytes directly to file instead of returning them as a char* and then writing them to file
//ToDo: Procedurally check all dates and times
//ToDo: Add support for directories
//Todo: Make DEFLATE create blocks with BLOCK_SIZE symbols


// Reminder : characters are encoded using a canonical huffman tree. This tree is encoded by only writing the code
// lengths of the alphabet (which is the entire ASCII table). This is enough to be able to build the entire tree when
// decoding the file. But the code lengths are themselves encoded using a huffman code defined in the DEFLATE
// specifications.