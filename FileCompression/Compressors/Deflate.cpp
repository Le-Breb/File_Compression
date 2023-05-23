#include "Deflate.h"
#include "Huffman_Tree/Huffman_Tree.h"
#include <exception>
#include <iostream>
#include <vector>

std::pair<bool, std::vector<char>> Deflate::decompress_block(Stream_Reader& reader)
{
    const bool is_final_block = reader.read_bits(1)[0];
    
    switch (reader.read_number(2))
    {
        case 0:
            return {is_final_block, get_stored_data(reader)};
        case 1:
            return {is_final_block, get_fixed_huffman_data(reader)};
        case 2:
            return {is_final_block, get_dynamic_huffman_data(reader)};
        case 3:
            throw std::runtime_error("Reserved block type!");
        default: // Should never happen
            throw std::runtime_error("Unknown block type!");
    }
}

std::vector<char> Deflate::get_stored_data(Stream_Reader& reader)
{
    reader.skip_end_of_byte();
    const auto len = static_cast<short>(reader.read_number(16));
    const auto nlen = static_cast<short>(reader.read_number(16));
    if (len != ~nlen)
        throw std::runtime_error("Invalid stored block length!");
    std::vector<char> bytes = reader.read_bytes_v(len);

    return bytes;
}


std::vector<char> Deflate::get_fixed_huffman_data(Stream_Reader& reader)
{
    std::vector<char> data;
    int lit_len = static_huffman_tree_->read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255)
            data.push_back(static_cast<char>(lit_len));
        else if (lit_len <= 287) // Length
        {
            int len = lengths_base_values_[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len])
                len += reader.read_number(extra_bits);
            // Distance
            const int dist_value = reader.read_number(5);
            if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");
            int dist = distances_base_values[dist_value];
            if (const int extra_bits = distances_extra_bits[dist_value])
                dist += reader.read_number(extra_bits);
            const int start_index = static_cast<int>(data.size()) - dist;
            for (int i = 0; i < len; i++)
                data.push_back(data[start_index + (i % dist)]);
        }
        else
            throw std::runtime_error("Invalid fixed huffman code!");

        lit_len = static_huffman_tree_->read_key(reader);
    }

    return data;
}

std::vector<char> Deflate::get_dynamic_huffman_data(Stream_Reader& reader)
{
    throw std::runtime_error("Not implemented yet!");
}

void Deflate::build_static_huffman_tree()
{
    std::map<int, std::pair<int, int>> keys_paths;
    int c = 0;
    for (int i = 0b00110000; i <= 0b10111111; i++)
        keys_paths[c++] = {i, 8};
    for (int i = 0b110010000; i <= 0b111111111; i++)
        keys_paths[c++] = {i, 9};
    for (int i = 0b0000000; i <= 0b0010111; i++)
        keys_paths[c++] = {i, 7};
    for (int i = 0b11000000; i <= 0b11000111; i++)
        keys_paths[c++] = {i, 8};

    static_huffman_tree_ = new Huffman_Tree(keys_paths);
}

std::pair<char*, int> Deflate::compress(const unsigned char* data, const int size)
{
    char* res = new char[size + 1];
    res[0] = 1; // Last block, no compression
    for (int i = 0; i < size; i++)
        res[i + 1] = static_cast<char>(data[i]);

    return {res, size + 1};
}

std::pair<char*, int> Deflate::decompress(const unsigned char* data, const int offset)
{
    // Build static huffman tree if not already built
    if (static_huffman_tree_ == nullptr)
        build_static_huffman_tree();
    try
    {
        int off = offset;
        std::vector<std::vector<char>> decompressed_blocks;
        Stream_Reader reader(data, offset);
        std::pair<bool, std::vector<char>> decompression;
        do
        {
            decompression = decompress_block(reader);
            decompressed_blocks.push_back(decompression.second);
            off += static_cast<int>(decompression.second.size());
        }
        while (!decompression.first);
        
        int decompressed_size = 0;
        for (const auto& decompressed_block : decompressed_blocks)
            decompressed_size += static_cast<int>(decompressed_block.size());

        char* uncompressed_data = new char[decompressed_size];

        off = 0;
        for (const auto& decompressed_block : decompressed_blocks)
        {
            for (const char c : decompressed_block)
                uncompressed_data[off++] = c;
        }

        return {uncompressed_data, decompressed_size};
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
        throw;
    }
}
    
    
