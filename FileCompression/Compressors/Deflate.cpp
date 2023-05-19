#include "Deflate.h"

#include <exception>
#include <iostream>
#include <vector>

std::tuple<bool, std::tuple<char*, int>> Deflate::decompress_block(StreamReader& reader)
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

std::tuple<char*, int> Deflate::get_stored_data(StreamReader& reader)
{
    reader.skip_end_of_byte();
    const auto len = static_cast<short>(reader.read_number(16));
    const auto nlen = static_cast<short>(reader.read_number(16));
    if (len != ~nlen)
        throw std::exception("Invalid stored block length!");
    char* bytes = reader.read_bytes(len);

    return {bytes, len};
}

std::tuple<char*, int> Deflate::get_fixed_huffman_data(StreamReader& reader)
{
    throw std::exception("Not implemented yet!");
}

std::tuple<char*, int> Deflate::get_dynamic_huffman_data(StreamReader& reader)
{
    throw std::exception("Not implemented yet!");
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
        StreamReader reader(data, offset);
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
    
    
