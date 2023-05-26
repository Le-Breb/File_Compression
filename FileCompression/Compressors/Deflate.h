#pragma once
#include <map>
#include <vector>

class Huffman_Tree;

class Deflate
{
public:
    /**
     * \brief Reader for compressed DEFLATE data.
     */
    class Stream_Reader
    {
        unsigned byte_offset_;
        unsigned bit_index_ = 0;
        const unsigned char* data_;
    public:
        Stream_Reader(const unsigned char* data, const unsigned offset) : byte_offset_{ offset }, data_ { data } {}

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

        bool read_bit()
        {
            const bool bit = data_[byte_offset_] & (1 << bit_index_++);
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

        std::vector<char> read_bytes_v(const int count)
        {
            std::vector<char> bytes;
            bytes.reserve(count);
            for (int i = 0; i < count; i++)
                bytes.push_back(static_cast<char>(data_[byte_offset_++]));

            return bytes;
        }

        char* read_bytes(const int count)
        {
            char* bytes = new char[count];
            for (int i = 0; i < count; i++)
                bytes[i] = static_cast<char>(data_[byte_offset_++]);

            return bytes;
        }
    };
    class Window {
        char data_[0x8000]{};
        int offset_ = 0;
        int size_ = 0;

    public:
        Window() = default;
        ~Window() = default;
        void add(const char& c) {
            data_[offset_++] = c;
            if (offset_ == 0x8000)
                offset_ = 0;
            if (size_ < 0x8000)
                size_++;

        }

        char get(const int offset) const {
            return data_[(offset_ - offset) % 0x8000];
        }

        int size() const { return size_; }
    };
    static std::pair<char*, int> deflate(const unsigned char* data, int size);
    static std::pair<char*, int> inflate(const unsigned char* data, int offset);

private:
    static std::pair<bool, std::vector<char>> decompress_block(Stream_Reader& reader, Window& window);
    static std::vector<char> get_stored_data(Stream_Reader& reader);
    static std::vector<char> get_fixed_huffman_data(Stream_Reader& reader, Window& window);
    static std::vector<char> get_dynamic_huffman_data(Stream_Reader& reader, Window& window);
    static std::vector<char> read_dynamic_huffman_data(Stream_Reader& reader, Huffman_Tree& lit_len_tree, Huffman_Tree& dist_tree, Window& window);
    inline static Huffman_Tree* static_huffman_tree_ = nullptr;
    static void build_static_huffman_tree();
    static int*
    codes_from_code_lengths(const int code_lengths[], int num_codes, int max_code_length);
    static inline std::map<int, int> lengths_base_values_ = {
        { 257, 3 },{ 258, 4 },{ 259, 5 },{ 260, 6 },{ 261, 7 },{ 262, 8 },{ 263, 9 },{ 264, 10 },{ 265, 11 },{ 266, 13 },{ 267, 15 },{ 268, 17 },{ 269, 19 },{ 270, 23 },{ 271, 27 },{ 272, 31 },{ 273, 35 },{ 274, 43 },{ 275, 51 },{ 276, 59 },{ 277, 67 },{ 278, 83 },{ 279, 99 },{ 280, 115 },{ 281, 131 },{ 282, 163 },{ 283, 195 },{ 284, 227 },{ 285, 258 }
    };
    static inline std::map<int, int> lengths_extra_bits = {
        { 257, 0 },{ 258, 0 },{ 259, 0 },{ 260, 0 },{ 261, 0 },{ 262, 0 },{ 263, 0 },{ 264, 0 },{ 265, 1 },{ 266, 1 },{ 267, 1 },{ 268, 1 },{ 269, 2 },{ 270, 2 },{ 271, 2 },{ 272, 2 },{ 273, 3 },{ 274, 3 },{ 275, 3 },{ 276, 3 },{ 277, 4 },{ 278, 4 },{ 279, 4 },{ 280, 4 },{ 281, 5 },{ 282, 5 },{ 283, 5 },{ 284, 5 },{ 285, 0 }
    };
    static inline std::map<int, int> distances_base_values = {
        { 0, 1 },{ 1, 2 },{ 2, 3 },{ 3, 4 },{ 4, 5 },{ 5, 7 },{ 6, 9 },{ 7, 13 },{ 8, 17 },{ 9, 25 },{ 10, 33 },{ 11, 49 },{ 12, 65 },{ 13, 97 },{ 14, 129 },{ 15, 193 },{ 16, 257 },{ 17, 385 },{ 18, 513 },{ 19, 769 },{ 20, 1025 },{ 21, 1537 },{ 22, 2049 },{ 23, 3073 },{ 24, 4097 },{ 25, 6145 },{ 26, 8193 },{ 27, 12289 },{ 28, 16385 },{ 29, 24577 }
    };
    static inline std::map<int, int> distances_extra_bits = {
        { 0, 0 },{ 1, 0 },{ 2, 0 },{ 3, 0 },{ 4, 1 },{ 5, 1 },{ 6, 2 },{ 7, 2 },{ 8, 3 },{ 9, 3 },{ 10, 4 },{ 11, 4 },{ 12, 5 },{ 13, 5 },{ 14, 6 },{ 15, 6 },{ 16, 7 },{ 17, 7 },{ 18, 8 },{ 19, 8 },{ 20, 9 },{ 21, 9 },{ 22, 10 },{ 23, 10 },{ 24, 11 },{ 25, 11 },{ 26, 12 },{ 27, 12 },{ 28, 13 },{ 29, 13 }
    };
    static inline int code_length_codes_order[] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    };
};
