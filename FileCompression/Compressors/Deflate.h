#pragma once
#include <tuple>
#include <vector>

class Deflate
{
    /**
     * \brief Reader for compressed DEFLATE data.
     */
    class StreamReader
    {
        unsigned byte_offset_;
        unsigned bit_index_ = 0;
        const unsigned char* data_;
    public:
        StreamReader(const unsigned char* data, const unsigned offset) : byte_offset_{ offset }, data_ { data } {}

        /**
         * \brief Moves the reader to the next byte
         */
        void skip_end_of_byte()
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
        std::vector<bool> read_bytes_forward(const int count)
        {
            std::vector<bool> bits = read_bits(count);
            std::reverse(bits.begin(), bits.end());

            return bits;
        }

        /**
         * \brief Reads a number of bits from the data in reverse order (first bit read is the LSB of the first byte)
         * \param count Number of bits to read
         */
        std::vector<bool> read_bits(const int count)
        {
            std::vector<bool> bits;
            bits.reserve(count);

            for (int i = 0; i < count; ++i)
            {
                bits.push_back(data_[byte_offset_] & (1 << bit_index_++));
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
         * \param bits Number of bits to read
         */
        int read_number(const int bits)
        {
            int n = 0;
            for (int i = 0; i < bits; i++)
            {
                n += ((data_[byte_offset_] & (1 << bit_index_)) >> bit_index_++) << i;
                if (bit_index_ == 8)
                {
                    bit_index_ = 0;
                    ++byte_offset_;
                }
            }

            return n;
        }

        char* read_bytes(const int count)
        {
            char* bytes = new char[count];
            for (int i = 0; i < count; i++)
                bytes[i] = data_[byte_offset_++];

            return bytes;
        }
    };
    
    static std::tuple<bool, std::tuple<char*, int>> decompress_block(StreamReader& reader);
    static std::tuple<char*, int> get_stored_data(StreamReader& reader);
    static std::tuple<char*, int> get_fixed_huffman_data(StreamReader& reader);
    static std::tuple<char*, int> get_dynamic_huffman_data(StreamReader& reader);
public:
    static std::tuple<char*, int> compress(const unsigned char* data, int size);
    static std::tuple<char*, int> decompress(const unsigned char* data, int offset);
};
