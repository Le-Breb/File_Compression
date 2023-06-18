//
// Created by matmu on 04/06/2023.
//

#ifndef FILECOMPRESSION_STREAMWRITER_H
#define FILECOMPRESSION_STREAMWRITER_H

#include <fstream>
#include <vector>

typedef unsigned char Byte;
namespace Deflate
{

    class StreamWriter
    {
        char curr_byte = 0;
        char bit_index_ = 0;
        std::ofstream out_;

        void writeCurrByte();

    public:
        StreamWriter(const char* path, unsigned offset);

        void writeNumber(int number, int bit_length);

        void writeCode(int code, int bit_length);

        void writeBit(bool bit);

        void writeBytes(const std::vector<bool>& bits);

        void writeBytes(const std::vector<Byte>& bytes);

        void close();
    };

} // Deflate

#endif //FILECOMPRESSION_STREAMWRITER_H
