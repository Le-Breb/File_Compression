#pragma once
#include <map>
#include <vector>
#include <unordered_map>
#include "Stream_Reader.h"
#include "Window.h"

namespace Deflate {
    class Huffman_Tree;

    class Main
    {
    public:

        static std::pair<char*, int> deflate(const char *data, int size);
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
        //inverse of lengths_base_values
        static inline std::unordered_map<int, int> length_codes = {
            { 3, 257 },{ 4, 258 },{ 5, 259 },{ 6, 260 },{ 7, 261 },{ 8, 262 },{ 9, 263 },{ 10, 264 },{ 11, 265 },{ 13, 266 },{ 15, 267 },{ 17, 268 },{ 19, 269 },{ 23, 270 },{ 27, 271 },{ 31, 272 },{ 35, 273 },{ 43, 274 },{ 51, 275 },{ 59, 276 },{ 67, 277 },{ 83, 278 },{ 99, 279 },{ 115, 280 },{ 131, 281 },{ 163, 282 },{ 195, 283 },{ 227, 284 },{ 258, 285 }
        };
        //inverse of distances_base_values
        static inline std::unordered_map<int, int> dist_codes = {
                { 1, 0 },{ 2, 1 },{ 3, 2 },{ 4, 3 },{ 5, 4 },{ 7, 5 },{ 9, 6 },{ 13, 7 },{ 17, 8 },{ 25, 9 },{ 33, 10 },{ 49, 11 },{ 65, 12 },{ 97, 13 },{ 129, 14 },{ 193, 15 },{ 257, 16 },{ 385, 17 },{ 513, 18 },{ 769, 19 },{ 1025, 20 },{ 1537, 21 },{ 2049, 22 },{ 3073, 23 },{ 4097, 24 },{ 6145, 25 },{ 8193, 26 },{ 12289, 27 },{ 16385, 28 },{ 24577, 29 }
        };
        static inline std::unordered_map<int, int> lengths_base_values_ = {
            { 257, 3 },{ 258, 4 },{ 259, 5 },{ 260, 6 },{ 261, 7 },{ 262, 8 },{ 263, 9 },{ 264, 10 },{ 265, 11 },{ 266, 13 },{ 267, 15 },{ 268, 17 },{ 269, 19 },{ 270, 23 },{ 271, 27 },{ 272, 31 },{ 273, 35 },{ 274, 43 },{ 275, 51 },{ 276, 59 },{ 277, 67 },{ 278, 83 },{ 279, 99 },{ 280, 115 },{ 281, 131 },{ 282, 163 },{ 283, 195 },{ 284, 227 },{ 285, 258 }
        };
        static inline std::unordered_map<int, int> lengths_extra_bits = {
            { 257, 0 },{ 258, 0 },{ 259, 0 },{ 260, 0 },{ 261, 0 },{ 262, 0 },{ 263, 0 },{ 264, 0 },{ 265, 1 },{ 266, 1 },{ 267, 1 },{ 268, 1 },{ 269, 2 },{ 270, 2 },{ 271, 2 },{ 272, 2 },{ 273, 3 },{ 274, 3 },{ 275, 3 },{ 276, 3 },{ 277, 4 },{ 278, 4 },{ 279, 4 },{ 280, 4 },{ 281, 5 },{ 282, 5 },{ 283, 5 },{ 284, 5 },{ 285, 0 }
        };
        static inline std::unordered_map<int, int> distances_base_values = {
            { 0, 1 },{ 1, 2 },{ 2, 3 },{ 3, 4 },{ 4, 5 },{ 5, 7 },{ 6, 9 },{ 7, 13 },{ 8, 17 },{ 9, 25 },{ 10, 33 },{ 11, 49 },{ 12, 65 },{ 13, 97 },{ 14, 129 },{ 15, 193 },{ 16, 257 },{ 17, 385 },{ 18, 513 },{ 19, 769 },{ 20, 1025 },{ 21, 1537 },{ 22, 2049 },{ 23, 3073 },{ 24, 4097 },{ 25, 6145 },{ 26, 8193 },{ 27, 12289 },{ 28, 16385 },{ 29, 24577 }
        };
        static inline std::unordered_map<int, int> distances_extra_bits = {
            { 0, 0 },{ 1, 0 },{ 2, 0 },{ 3, 0 },{ 4, 1 },{ 5, 1 },{ 6, 2 },{ 7, 2 },{ 8, 3 },{ 9, 3 },{ 10, 4 },{ 11, 4 },{ 12, 5 },{ 13, 5 },{ 14, 6 },{ 15, 6 },{ 16, 7 },{ 17, 7 },{ 18, 8 },{ 19, 8 },{ 20, 9 },{ 21, 9 },{ 22, 10 },{ 23, 10 },{ 24, 11 },{ 25, 11 },{ 26, 12 },{ 27, 12 },{ 28, 13 },{ 29, 13 }
        };
        static inline int code_length_codes_order[] = {
            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };
    };
}

