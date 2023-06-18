//
// Created by matmu on 28/05/2023.
//

#ifndef FILECOMPRESSION_STREAMREADER_H
#define FILECOMPRESSION_STREAMREADER_H

#include <vector>

typedef unsigned char Byte;
namespace Deflate
{
    /**
         * \brief Reader for compressed DEFLATE data.
         */
    class StreamReader
    {
        unsigned byte_offset_;
        unsigned bit_index_ = 0;
        const std::vector<Byte>* data_;
    public:
        explicit StreamReader(const std::vector<Byte>* data) : byte_offset_{0}, data_{data}
        {}

        /**
         * \brief Moves the reader to the next byte
         */
        void skipEndOfByte()
        {
            if (bit_index_ != 0)
            {
                bit_index_ = 0;
                ++byte_offset_;
            }
        }

        /**
         * \brief Reads a number of bits from the data in reverse order (first bit read is the LSB of the first byte)
         * \param count Number of bits to read
         */
        std::vector<bool> readBytesForward(const int count)
        {
            std::vector<bool> bits = readBits(count);
            std::reverse(bits.begin(), bits.end());

            return bits;
        }

        bool readBit()
        {
            const bool bit = (*data_)[byte_offset_] & (1 << bit_index_++);
            if (bit_index_ == 8)
            {
                bit_index_ = 0;
                ++byte_offset_;
            }

            return bit;
        }

        /**
         * \brief Reads a number of bits from the data in reverse order (first bit read is the LSB of the first byte)
         * \param count Number of bits to read
         */
        std::vector<bool> readBits(const int count)
        {
            std::vector<bool> bits;
            bits.reserve(count);

            for (int i = 0; i < count; ++i)
            {
                bits.push_back((*data_)[byte_offset_] & (1 << bit_index_++));
                if (bit_index_ == 8)
                {
                    bit_index_ = 0;
                    ++byte_offset_;
                }
            }

            return bits;
        }

        /**
         * \brief Reads a number from the data
         * \param bit_length Number of bit_length to read
         */
        int readNumber(const int bit_length)
        {
            int n = 0;
            for (int i = 0; i < bit_length; i++)
            {
                n += (((*data_)[byte_offset_] & (1 << bit_index_)) >> bit_index_++) << i;
                if (bit_index_ == 8)
                {
                    bit_index_ = 0;
                    ++byte_offset_;
                }
            }

            return n;
        }

        std::vector<Byte> readBytesV(const int count)
        {
            std::vector<Byte> bytes;
            bytes.reserve(count);
            for (int i = 0; i < count; i++)
                bytes.push_back(static_cast<char>((*data_)[byte_offset_++]));

            return bytes;
        }

        char* readBytes(const int count)
        {
            char* bytes = new char[count];
            for (int i = 0; i < count; i++)
                bytes[i] = static_cast<char>((*data_)[byte_offset_++]);

            return bytes;
        }
    };
}

#endif //FILECOMPRESSION_STREAMREADER_H
