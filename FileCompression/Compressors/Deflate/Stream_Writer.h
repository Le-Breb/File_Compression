//
// Created by matmu on 04/06/2023.
//

#ifndef FILECOMPRESSION_STREAM_WRITER_H
#define FILECOMPRESSION_STREAM_WRITER_H

#include <fstream>
#include <vector>

typedef unsigned char Byte;
namespace Deflate
{

    class Stream_Writer
    {
        char curr_byte = 0;
        char bit_index_ = 0;
        std::ofstream out_;

        void write_curr_byte();

    public:
        Stream_Writer(const char* path, unsigned offset);

        void write_number(int number, int bit_length);

        void write_code(int code, int bit_length);

        void write_bit(bool bit);

        void write_bytes(const std::vector<bool>& bits);

        void write_bytes(const std::vector<Byte>& bytes);

        void close();
    };

} // Deflate

#endif //FILECOMPRESSION_STREAM_WRITER_H
