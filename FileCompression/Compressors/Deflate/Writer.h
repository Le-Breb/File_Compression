//
// Created by matmu on 04/06/2023.
//

#ifndef FILECOMPRESSION_WRITER_H
#define FILECOMPRESSION_WRITER_H

#include <vector>

namespace Deflate
{

    class Writer
    {
        unsigned char curr_byte = 0;
        char bit_index_ = 0;

        void write_curr_byte();

    public:
        Writer();

        /**
         * \brief Writes a number of bits to the data in regular order
         * \param number Number to write
         * \param bit_length Length of the number in bits
         */
        void write_number(int number, int bit_length);

        /** \brief Writes a code to the data in bit-reversed order
         * \param number Code to write
         * \param bit_length Length of the code in bits
         */
        void write_code(int code, int bit_length);

        void write_bit(bool bit);

        void close();

        std::vector<unsigned char> data;
    };

} // Deflate

#endif //FILECOMPRESSION_WRITER_H
