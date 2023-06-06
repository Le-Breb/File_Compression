
#include <bitset>
#include <fstream>
#include <iostream>
#include <ostream>

#include "ZipFile.h"
#include "Base64.h"
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
    /*auto a = "Fci7DYAwDAXAVV5HxxD0DGGJh3CU2PnY+wPl3eFWPCcK0TbpnbUSp8SjTDD+X6kLaZ/yUrEADWq3zyahI7m/";
    auto b = Base64::decode(a);
    auto c = new unsigned char[b.size()];
    for (int i = 0; i < b.size(); ++i)
        c[i] = b[i];
    auto d = Deflate::Main::inflate(c);
    for (int i = 0; i < d.second; ++i)
        std::cout << d.first[i];*/
    const char* test = "Bonjour je m\'appelle Mathieu et je suis un etudiant en informatique.";
    auto res = Deflate::Main::deflate(test, static_cast<int>(strlen(test)));
    auto c = new unsigned char[res.second];
    for (int i = 0; i < res.second; ++i)
        c[i] = res.first[i];
    auto d = Deflate::Main::inflate(c);
    for (int i = 0; i < d.second; ++i)
        std::cout << d.first[i];
    std::cout<<std::endl;
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