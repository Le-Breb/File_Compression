#include "Deflate.h"

#include <exception>
#include <iostream>
#include <vector>

std::tuple<bool, std::tuple<char*, int>> Deflate::decompress_block(Stream_Reader& reader)
{
    std::tuple<char*, int> decompressed_block;
    const bool is_final_block = reader.read_bits(1)[0];
    
    switch (reader.read_number(2))
    {
        case 0:
            decompressed_block = get_stored_data(reader);
            break;
        case 1:
            decompressed_block = get_fixed_huffman_data(reader);
            break;
        case 2:
            decompressed_block = get_dynamic_huffman_data(reader);
            break;
        case 3:
            throw std::exception("Reserved block type!");
        default: // Should never happen
            throw std::exception("Unknown block type!");
    }


    return {is_final_block, decompressed_block};
}

std::tuple<char*, int> Deflate::get_stored_data(Stream_Reader& reader)
{
    reader.skip_end_of_byte();
    const auto len = static_cast<short>(reader.read_number(16));
    const auto nlen = static_cast<short>(reader.read_number(16));
    if (len != ~nlen)
        throw std::exception("Invalid stored block length!");
    char* bytes = reader.read_bytes(len);

    return {bytes, len};
}

std::tuple<char*, int> Deflate::get_fixed_huffman_data(Stream_Reader& reader)
{
    int lit_len = 1;
}

std::tuple<char*, int> Deflate::get_dynamic_huffman_data(Stream_Reader& reader)
{
    throw std::exception("Not implemented yet!");
}

void Deflate::build_static_huffman_tree()
{
    std::map<char, std::pair<int, int>> keys_paths;
    for (int i = 0b00110000; i <= 10111111; i++)
        keys_paths[static_cast<char>(i)] = {i, 8};
    for (int i = 0b110010000; i <= 111111111; i++)
        keys_paths[static_cast<char>(i)] = {i, 9};
    for (int i = 0b0000000; i <= 0010111; i++)
        keys_paths[static_cast<char>(i)] = {i, 7};
    for (int i = 0b11000000; i <= 11000111; i++)
        keys_paths[static_cast<char>(i)] = {i, 8};

    static_huffman_tree_ = Huffman_Tree(keys_paths);
}

std::tuple<char*, int> Deflate::compress(const unsigned char* data, const int size)
{
    char* res = new char[size + 1];
    res[0] = 1; // Last block, no compression
    for (int i = 0; i < size; i++)
        res[i + 1] = data[i];

    return {res, size + 1};
}

std::tuple<char*, int> Deflate::decompress(const unsigned char* data, const int offset)
{
    try
    {
        int off = offset;
        std::vector<std::tuple<char*, int>> decompressed_blocks;
        Stream_Reader reader(data, offset);
        std::tuple<bool, std::tuple<char*, int>> decompression;
        do
        {
            decompression = decompress_block(reader);
            decompressed_blocks.push_back(std::get<1>(decompression));
            off += std::get<1>(std::get<1>(decompression));
        }
        while (!std::get<0>(decompression));
        
        int decompressed_size = 0;
        for (auto decompressed_block : decompressed_blocks)
            decompressed_size += std::get<1>(decompressed_block);

        char* uncompressed_data = new char[decompressed_size];

        off = 0;
        for (auto decompressed_block : decompressed_blocks)
        {
            for (int i = 0; i < std::get<1>(decompressed_block); i++)
                uncompressed_data[off++] = std::get<0>(decompressed_block)[i];
        }

        return {uncompressed_data, decompressed_size};
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
        throw;
    }
}
    
    
