#pragma once

#include <map>
#include <vector>
#include <unordered_map>
#include <list>
#include "Stream_Reader.h"
#include "Window.h"
#include "Match.h"
#include "Writer.h"
#include "Huffman_Tree.h"

typedef unsigned char Byte;
namespace Deflate
{
    class Memory;

    class Main
    {
    public:
        static const int MAX_CHAIN_LENGTH = 1024;
        static const int MAX_CODE_LENGTH = 15;
        static const int MAX_CODE_LENGTH_CODE_LENGTH = 7;
        static const int MAX_SYMBOLS_PER_BLOCK = 16384;
        static const int mem_level = 8;
        static const int hash_bits = mem_level + 7;
        static const int hash_size = 1 << hash_bits;
        static const int hash_mask = hash_size - 1;
        static const int hash_shift = (hash_bits + 3 - 1) / 3;
        static const int window_bits = 15;
        static const int window_size = 1 << window_bits;
        static const int window_mask = window_size - 1;

        //static const int lit_bufsize = 1 << (mem_level + 6);
        static std::vector<Byte> deflate(const Byte* data, int size);

        static std::vector<Byte> inflate(std::vector<Byte> data);

        static void Test();

        static void Test_file(const std::string& file_name, bool verify_compression);

    private:
        /** \brief Limit of searches in a hash chains. Reduces compression efficiency but makes it faster */


        struct dynamic_comp_res
        {
            dynamic_comp_res(int offset, int uncompressedSize, int providedLitLen,
                             int providedDistCodes, int numCodeLengthCodeLengthToWrite,
                             const std::vector<std::pair<int, int>>* litLenCodeLengthsToWrite,
                             const std::vector<std::pair<int, int>>* distCodeLengthsToWrite,
                             const Huffman_Tree::Code* codeLengthCodes, const Deflate::Huffman_Tree::Code* litLenCodes,
                             const Deflate::Huffman_Tree::Code* distCodes, const std::list<Match>* matches,
                             bool is_last_block);

            virtual ~dynamic_comp_res();

            const int offset;
            const int uncompressed_size;
            const int provided_lit_len;
            const int provided_dist_codes;
            const int num_code_length_code_length_to_write;
            const std::vector<std::pair<int, int>>* lit_len_code_lengths_to_write;
            const std::vector<std::pair<int, int>>* dist_code_lengths_to_write;
            const Deflate::Huffman_Tree::Code* code_length_codes;
            const Deflate::Huffman_Tree::Code* lit_len_codes;
            const Deflate::Huffman_Tree::Code* dist_codes;
            const std::list<Match>* matches;
            const bool is_last_block;
        };

        /** \brief Deflates a block of data using the dynamic Huffman codes. <br>
         * <b>This method must be called only when the output size is known and
         * inferior to the fixed output size.</b>
         * @param dynamic_comp_res The dynamic compression result
         * @param writer The writer used to write the compressed data
         * @param data Input data
         * @param offset Data offset.
         * @return compressed_size The size of the compressed data
         */
        static void deflate_dynamic(const dynamic_comp_res& dynamic_comp_res, Deflate::Writer& writer, const Byte* data,
                                    int compressed_size);

        /** \brief Computes the number of bits required to encode a given
         * literal/length code with fixed huffman codes. */
        static int lit_len_fixed_code_length(int lit_len);

        /** Computes the number of bytes required to compress a block with fixed huffman codes.
         * @param data The data to compress
         * @param dynamic_comp_res The dynamic compression result
         * */
        static int compressed_size_with_fixed_codes(const Byte* data, const dynamic_comp_res& dynamic_comp_res);

        /** \brief Computes the number of bytes required to compress a block with dynamic huffman codes.
         * @param data The data to compress
         * @param dynamic_comp_res The dynamic compression result
         * */
        static int
        compressed_size_with_dynamic_codes(const Byte* data,
                                           const Deflate::Main::dynamic_comp_res& dynamic_comp_res);

        /** \brief Computes the data necessary to compress a block with dynamic huffman coding */
        static Deflate::Main::dynamic_comp_res
        compute_dynamic_comp_data(const Byte* data, int data_size, Writer& writer, const int offset,
                                  Deflate::Memory& mem);

        /** \brief Deflates a block of data using the fixed Huffman codes. <br>
         * <b>This method must be called only when the output size is known and
         * inferior to the dynamic output size.</b>
         * @param data The data to deflate
         * @param writer The writer used to write the compressed data
         * @param dynamic_comp_res The dynamic compression result
         */
        static void
        deflate_fixed(const Byte* data, Deflate::Writer& writer, const dynamic_comp_res& dynamic_comp_res,
                      int compressed_size);


        /** \brief Deflates a block of data using the uncompressed block format. <br>
         * <b>This method must be called only when the output size is known and
         * both dynamic and fixed output size are superior to input size.</b>
         * @param data The data to deflate
         * @param num_bytes The number of bytes to deflate
         * @param offset The offset of the data
         * @param is_last_block Whether this is the last block
         * @param writer The writer used to write the compressed data
         */
        static void
        deflate_uncompressed(const Byte* data, int num_bytes, int offset, bool is_last_block,
                             Writer& writer);

        /** \brief Writes the data of a dynamic block */
        static void
        write_compressed_data(Deflate::Writer& writer, const Byte* data, int offset, int size,
                              const std::list<Match>& matches,
                              const Deflate::Huffman_Tree::Code* lit_len_codes,
                              const Deflate::Huffman_Tree::Code* distance_codes);

        /** \brief Writes the code lengths of a dynamic block */
        static void
        write_code_lengths(Deflate::Writer& writer, const std::vector<std::pair<int, int>>& code_lengths,
                           const Deflate::Huffman_Tree::Code* code_length_codes);

        static int
        enumerate_code_lengths(const int count, const Deflate::Huffman_Tree::Code* codes, const int max_repetition,
                               std::vector<std::pair<int, int>>& code_lengths_to_write,
                               Memory& mem);

        /** \brief Builds a tree given the code lengths of the symbols */
        static Huffman_Tree*
        code_lengths_to_tree(const int code_lengths[], int num_symbols, int max_length);

        /** \brief Reads the code lengths of a dynamic block */
        static void
        read_code_lengths(Deflate::Stream_Reader& reader, const Deflate::Huffman_Tree* tree, int num_symbols,
                          int& max_length,
                          int code_lengths[]);


        /** \brief Computes the code of the value range in which the given length is */
        static int length_to_length_code(int length);

        /** \brief Computes the code of the value range in which the given distance is */
        static int distance_to_distance_code(int distance);

        static int
        compute_dynamic_trees(const Byte* data, int offset, int size, std::list<Match>& matches,
                              Huffman_Tree*& lit_len_tree,
                              Huffman_Tree*& dist_tree, Memory& mem);

        /** \brief Decompresses a block of data
         * @param reader A DEFLATE reader for the compressed data
         * @param window The window to use for decompression of the whole file
         * @param writer The writer used to write the decompressed data
         * @return A bool indicating whether the block was the last one
         */
        static bool
        decompress_block(Stream_Reader& reader, Window& window, Writer& writer);

        /** \brief Reads a DEFLATE stored block */
        static void get_stored_data(Stream_Reader& reader, Writer& writer);

        /** \brief Reads a DEFLATE block compressed with fixed Huffman coding */
        static void get_fixed_huffman_data(Stream_Reader& reader, Window& window, Writer& writer);

        /** \brief Reads a DEFLATE block compressed with dynamic Huffman coding */
        static void get_dynamic_huffman_data(Stream_Reader& reader, Window& window, Writer& writer);

        /** \brief Reads compressed data of DEFLATE block compressed with dynamic Huffman coding
         * @param reader A DEFLATE reader for the compressed data
         * @param lit_len_tree The Huffman tree for the literal and length codes
         * @param dist_tree The Huffman tree for the distance codes
         * @param window The window to use for decompression of the whole file
         * */
        static void
        read_dynamic_huffman_data(Stream_Reader& reader, const Huffman_Tree* lit_len_tree,
                                  const Huffman_Tree* dist_tree,
                                  Window& window,
                                  Writer& writer);

        /** \brief The DEFLATE static Huffman tree */
        inline static Huffman_Tree* static_huffman_tree_ = nullptr;

        /** \brief Computes the DEFLATE fixed Huffman tree */
        static void build_fixed_huffman_tree();

        /** \brief Computes the DEFLATE fixed Huffman codes */
        static void build_fixed_huffman_lit_len_values_codes();

        /// \brief Computes codes from code lengths using the <b>algorithm described in RFC 1951 section 3.2.2</b>
        static int* codes_from_code_lengths(const int code_lengths[], int num_symbols, int max_code_length);


        static inline void update_hash(int& h, Byte c)
        { h = (((h) << hash_shift) ^ (c)) & hash_mask; }

        static inline std::unordered_map<int, int> fixed_lit_len_values_codes;
        //inverse of lengths_base_values
        static inline std::map<int, int> length_codes = {
                {3,   257},
                {4,   258},
                {5,   259},
                {6,   260},
                {7,   261},
                {8,   262},
                {9,   263},
                {10,  264},
                {11,  265},
                {13,  266},
                {15,  267},
                {17,  268},
                {19,  269},
                {23,  270},
                {27,  271},
                {31,  272},
                {35,  273},
                {43,  274},
                {51,  275},
                {59,  276},
                {67,  277},
                {83,  278},
                {99,  279},
                {115, 280},
                {131, 281},
                {163, 282},
                {195, 283},
                {227, 284},
                {258, 285}
        };
        //inverse of dist_code_to_dist
        static inline std::map<int, int> dist_codes = {
                {1,     0},
                {2,     1},
                {3,     2},
                {4,     3},
                {5,     4},
                {7,     5},
                {9,     6},
                {13,    7},
                {17,    8},
                {25,    9},
                {33,    10},
                {49,    11},
                {65,    12},
                {97,    13},
                {129,   14},
                {193,   15},
                {257,   16},
                {385,   17},
                {513,   18},
                {769,   19},
                {1025,  20},
                {1537,  21},
                {2049,  22},
                {3073,  23},
                {4097,  24},
                {6145,  25},
                {8193,  26},
                {12289, 27},
                {16385, 28},
                {24577, 29}
        };
        static inline std::unordered_map<int, int> lit_len_code_to_length = {
                {257, 3},
                {258, 4},
                {259, 5},
                {260, 6},
                {261, 7},
                {262, 8},
                {263, 9},
                {264, 10},
                {265, 11},
                {266, 13},
                {267, 15},
                {268, 17},
                {269, 19},
                {270, 23},
                {271, 27},
                {272, 31},
                {273, 35},
                {274, 43},
                {275, 51},
                {276, 59},
                {277, 67},
                {278, 83},
                {279, 99},
                {280, 115},
                {281, 131},
                {282, 163},
                {283, 195},
                {284, 227},
                {285, 258}
        };
        static inline std::unordered_map<int, int> lengths_extra_bits = {
                {257, 0},
                {258, 0},
                {259, 0},
                {260, 0},
                {261, 0},
                {262, 0},
                {263, 0},
                {264, 0},
                {265, 1},
                {266, 1},
                {267, 1},
                {268, 1},
                {269, 2},
                {270, 2},
                {271, 2},
                {272, 2},
                {273, 3},
                {274, 3},
                {275, 3},
                {276, 3},
                {277, 4},
                {278, 4},
                {279, 4},
                {280, 4},
                {281, 5},
                {282, 5},
                {283, 5},
                {284, 5},
                {285, 0}
        };
        static inline std::unordered_map<int, int> dist_code_to_dist = {
                {0,  1},
                {1,  2},
                {2,  3},
                {3,  4},
                {4,  5},
                {5,  7},
                {6,  9},
                {7,  13},
                {8,  17},
                {9,  25},
                {10, 33},
                {11, 49},
                {12, 65},
                {13, 97},
                {14, 129},
                {15, 193},
                {16, 257},
                {17, 385},
                {18, 513},
                {19, 769},
                {20, 1025},
                {21, 1537},
                {22, 2049},
                {23, 3073},
                {24, 4097},
                {25, 6145},
                {26, 8193},
                {27, 12289},
                {28, 16385},
                {29, 24577}
        };
        static inline std::unordered_map<int, int> distances_extra_bits = {
                {0,  0},
                {1,  0},
                {2,  0},
                {3,  0},
                {4,  1},
                {5,  1},
                {6,  2},
                {7,  2},
                {8,  3},
                {9,  3},
                {10, 4},
                {11, 4},
                {12, 5},
                {13, 5},
                {14, 6},
                {15, 6},
                {16, 7},
                {17, 7},
                {18, 8},
                {19, 8},
                {20, 9},
                {21, 9},
                {22, 10},
                {23, 10},
                {24, 11},
                {25, 11},
                {26, 12},
                {27, 12},
                {28, 13},
                {29, 13}
        };
        static inline int code_length_codes_order[] = {
                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };
    };
}