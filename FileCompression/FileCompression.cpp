
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

/* https://en.wikipedia.org/wiki/ZIP_(file_format)
 * https://en.wikipedia.org/wiki/Deflate
 * https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
 * https://www.ietf.org/rfc/rfc1951.txt
 * http://abcdrfc.free.fr/rfc-vf/rfc1951.html
 * https://blog.za3k.com/understanding-gzip-2/
 * http://pnrsolution.org/Datacenter/Vol4/Issue1/58.pdf
 * https://jnior.com/deflate-compression-algorithm/
 * https://github.dev/madler/zlib/blob/master/deflate.c#L2012
 * https://www.euccas.me/zlib/#:~:text=In%20zlib%2C%20the%20default%20size%20of%20sliding%20window%20is%2064KB.
 * https://www.zlib.net/manual.html
*/


//ToDo: Writing and reading bytes directly from an to file instead of storing them in a char array
//ToDo: Procedurally check all dates and times
//Todo: Reorder computeDynamicTrees
//Todo: Tester le cas où il y a plus d138 0s à la suite dans enumerateCodeLengths
//Todo: Benchmark ce commit et le précédent pour voir en quoi celui-ci est plus lent
//Todo: Faire un buffer pour les codes lenghts code lengths avec une classe dédiée ce qui permettrait de un
// D'éviter de nomreuses allocations et désallocations mais aussi de ne plus avoir besoin de lit_len_code_lengths_to_write
// et dist_code_lengths_to_read
int main(int argc, char* argv[])
{
    Deflate::Main::testFile(
            R"(C:\Users\matmu\Documents\Unity\Projects\project-s2\MuseumLeapInstaller.exe)", true);

    Deflate::Main::test();
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
