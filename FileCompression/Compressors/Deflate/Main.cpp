#include "Main.h"
#include "Huffman_Tree.h"
#include "Window.h"
#include "Match.h"
#include "Writer.h"
#include <exception>
#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <iomanip>

std::pair<bool, std::vector<unsigned char>> Deflate::Main::decompress_block(Stream_Reader& reader, Window& window)
{
    const bool is_final_block = reader.read_bit();

    switch (reader.read_number(2))
    {
        case 0:
            return {is_final_block, get_stored_data(reader)};
        case 1:
            return {is_final_block, get_fixed_huffman_data(reader, window)};
        case 2:
            return {is_final_block, get_dynamic_huffman_data(reader, window)};
        case 3:
            throw std::runtime_error("Reserved block type!");
        default: // Should never happen
            throw std::runtime_error("Unknown block type!");
    }
}

std::vector<unsigned char> Deflate::Main::get_stored_data(Stream_Reader& reader)
{
    reader.skip_end_of_byte();
    const auto len = static_cast<short>(reader.read_number(16));
    const auto nlen = static_cast<short>(reader.read_number(16));
    if (len != ~nlen)
        throw std::runtime_error("Invalid stored block length!");
    std::vector<unsigned char> bytes = reader.read_bytes_v(len);

    return bytes;
}

std::vector<unsigned char> Deflate::Main::get_fixed_huffman_data(Stream_Reader& reader, Window& window)
{
    std::vector<unsigned char> data;
    int lit_len = static_huffman_tree_->read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255)
        {
            data.push_back(static_cast<char>(lit_len));
            window.add(static_cast<char>(lit_len));
        }
        else if (lit_len <= 287) // Length
        {
            int len = lit_len_code_to_length[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len])
                len += reader.read_number(extra_bits);
            // Distance
            const int dist_value = reader.read_number(5);
            if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");
            int dist = distance_code_to_distance[dist_value];
            if (const int extra_bits = distance_extra_bits[dist_value])
                dist += reader.read_number(extra_bits);
            for (int i = 0; i < len; ++i)
            {
                char c = window.get(dist); // Current pos increases when we add to window so the offset is always dist
                data.push_back(c);
                window.add(c);
            }
        }
        else
            throw std::runtime_error("Invalid fixed huffman code!");

        lit_len = static_huffman_tree_->read_key(reader);
    }

    return data;
}

std::map<int, std::pair<int, int>> code_length_codes_verif;
std::map<int, std::pair<int, int>> lit_len_codes_verif;
std::map<int, std::pair<int, int>> distance_codes_verif;

std::vector<unsigned char> Deflate::Main::get_dynamic_huffman_data(Stream_Reader& reader, Window& window)
{
    std::vector<unsigned char> data;
    const int num_lit_len = reader.read_number(5) + 257;
    const int num_dist = reader.read_number(5) + 1;
    const int num_code_length_code_length = reader.read_number(4) + 4;
    int code_length_code_lengths[19];

    // Read code length code lengths
    int max_code_length_code_length = 0;
    for (int i = 0; i < num_code_length_code_length; ++i)
    {
        int code_length_code_length = reader.read_number(3);
        code_length_code_lengths[code_length_codes_order[i]] = code_length_code_length;
        if (code_length_code_length > max_code_length_code_length)
            max_code_length_code_length = code_length_code_length;
    }
    for (int i = num_code_length_code_length; i < 19; ++i)
        code_length_code_lengths[code_length_codes_order[i]] = 0;

    // Build code length tree from code length code lengths
    int* code_length_codes = codes_from_code_lengths(code_length_code_lengths, 19, max_code_length_code_length);
    std::map<int, std::pair<int, int>> code_length_keys_paths;
    for (int i = 0; i < 19; ++i)
    {
        if (code_length_codes[i] != -1)
        {
            code_length_keys_paths[i] = {code_length_codes[i], code_length_code_lengths[i]};
            if (code_length_codes_verif[i] != code_length_keys_paths[i])
                throw std::runtime_error("Code length code error!");
        }
    }
    Huffman_Tree code_lengths_tree(code_length_keys_paths);
    delete code_length_codes;

    // Read lit_len code lengths
    int read_codes = 0;
    int lit_len_code_lengths[num_lit_len];
    int max_lit_len_code_length = 0;
    while (read_codes < num_lit_len)
    {
        const int code = code_lengths_tree.read_key(reader);
        if (code < 16)
            lit_len_code_lengths[read_codes++] = code;
        else if (code == 16)
        {
            const int repeat = reader.read_number(2) + 3;
            for (int i = 0; i < repeat; ++i)
                lit_len_code_lengths[read_codes++] = lit_len_code_lengths[read_codes - 1];
        }
        else if (code == 17)
        {
            const int repeat = reader.read_number(3) + 3;
            for (int i = 0; i < repeat; ++i)
                lit_len_code_lengths[read_codes++] = 0;
        }
        else if (code == 18)
        {
            const int repeat = reader.read_number(7) + 11;
            for (int i = 0; i < repeat; ++i)
                lit_len_code_lengths[read_codes++] = 0;
        }
        else
            throw std::runtime_error("Invalid code length code!");
        if (lit_len_code_lengths[read_codes - 1] > max_lit_len_code_length)
            max_lit_len_code_length = lit_len_code_lengths[read_codes - 1];
    }

    // Build lit_len tree from lit_len code lengths
    int* lit_len_codes = codes_from_code_lengths(lit_len_code_lengths, num_lit_len, max_lit_len_code_length);
    std::map<int, std::pair<int, int>> lit_len_codes_keys_paths;
    for (int i = 0; i < num_lit_len; ++i)
    {
        if (lit_len_codes[i] != -1)
        {
            lit_len_codes_keys_paths[i] = {lit_len_codes[i], lit_len_code_lengths[i]};
            if (lit_len_codes_verif[i] != lit_len_codes_keys_paths[i])
                throw std::runtime_error("Lit len code error!");
        }
    }
    Huffman_Tree lit_len_tree(lit_len_codes_keys_paths);
    delete lit_len_codes;

    // Read dist code lengths
    read_codes = 0;
    int dist_code_lengths[num_dist];
    int max_dist_code_length = 0;
    while (read_codes < num_dist)
    {
        const int code = code_lengths_tree.read_key(reader);
        if (code < 16)
            dist_code_lengths[read_codes++] = code;
        else if (code == 16)
        {
            const int repeat = reader.read_number(2) + 3;
            for (int i = 0; i < repeat; ++i)
                dist_code_lengths[read_codes++] = dist_code_lengths[read_codes - 1];
        }
        else if (code == 17)
        {
            const int repeat = reader.read_number(3) + 3;
            for (int i = 0; i < repeat; ++i)
                dist_code_lengths[read_codes++] = 0;
        }
        else if (code == 18)
        {
            const int repeat = reader.read_number(7) + 11;
            for (int i = 0; i < repeat; ++i)
                dist_code_lengths[read_codes++] = 0;
        }
        else
            throw std::runtime_error("Invalid code length code!");
        if (dist_code_lengths[read_codes - 1] > max_dist_code_length)
            max_dist_code_length = dist_code_lengths[read_codes - 1];
    }

    // Build dist tree from dist code lengths
    int* distance_codes = codes_from_code_lengths(dist_code_lengths, num_dist, max_dist_code_length);
    std::map<int, std::pair<int, int>> dist_codes_keys_paths;
    for (int i = 0; i < num_dist; ++i)
    {
        if (distance_codes[i] != -1)
        {
            dist_codes_keys_paths[i] = {distance_codes[i], dist_code_lengths[i]};
            if (distance_codes_verif[i] != dist_codes_keys_paths[i])
                throw std::runtime_error("Dist code error!");
        }
    }
    Huffman_Tree dist_tree(dist_codes_keys_paths);
    delete distance_codes;

    return read_dynamic_huffman_data(reader, lit_len_tree, dist_tree, window);
}

void Deflate::Main::build_static_huffman_tree()
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

std::pair<unsigned char*, int> Deflate::Main::deflate(const unsigned char* data, int size)
{
    // Spot matches
    std::list<Match*> matches = find_matches(data, size);

    // Build dynamic huffman trees
    std::unordered_map<int, int> lit_len_frequency_table;
    std::unordered_map<int, int> dist_frequency_table;
    int index = 0;
    int nextMatchPos = !matches.empty() ? matches.front()->position() : size;
    auto nextMatchIter = matches.begin();
    while (index < size)
    {
        if (index == nextMatchPos)
        {
            int length_code = length_to_code((*nextMatchIter)->length());
            int dist_code = distance_to_code((*nextMatchIter)->distance());

            lit_len_frequency_table[length_code]++;
            dist_frequency_table[dist_code]++;

            index += (*nextMatchIter)->length();
            nextMatchPos = ++nextMatchIter == matches.end() ? size : (*nextMatchIter)->position();
        }
        else
        {
            lit_len_frequency_table[data[index]]++;
            index++;
        }
    }
    lit_len_frequency_table[256] = 1; // End of block
    if (dist_frequency_table.empty()) // No match -> We provide a distance of 0
    {
        // Two distance codes are required to build a tree
        dist_frequency_table[0] = 1;
        dist_frequency_table[1] = 1;
    }
    Huffman_Tree lit_len_tree(lit_len_frequency_table);
    Huffman_Tree dist_tree(dist_frequency_table);

    /* Steps :
     * Build a list of code length for each lit/len and dist
     * Enumerate them and store how many times we need to say the raw length, how many
     * times we can duplicate the last one 3-6 times, how many times in a row the code 0 appears
     * (for both ranges 3-10 and 11-138).
     * Build a huffman tree out of those 19 values (0-15 => raw length, 16 => 3-6 repeat, 17 => 3-10 0, 18 => 11-138 0)
     * Write the 19 code lengths int the order 16 17 18 0 8 7 9 6 10 5 11 4 12 3 13 2 14 1 15
     * */
    int lit_len_code_lengths[286];
    int dist_code_lengths[30];
    std::map<int, std::pair<int, int>> lit_len_codes = lit_len_tree.canonical_codes(15);
    lit_len_codes_verif = lit_len_codes;
    std::map<int, std::pair<int, int>> distance_codes = dist_tree.canonical_codes(15);
    distance_codes_verif = distance_codes;
    for (int& lit_len_code_length : lit_len_code_lengths)
        lit_len_code_length = 0;
    for (int& dist_code_length : dist_code_lengths)
        dist_code_length = 0;
    for (const auto& [c, code_len] : lit_len_codes)
        lit_len_code_lengths[c] = code_len.second;
    for (const auto& [c, code_len] : distance_codes)
        dist_code_lengths[c] = code_len.second;
    std::unordered_map<int, int> code_lengths_frequency_table;
    /** \brief Code lengths to write <br>
     * 0-15 : raw length <br>
     * 16 : 3-6 repeat <br>
     * 17 : 3-10 0 <br>
     * 18 : 11-138 0 <br>
     * (code in [0, 18], length in [3, 138])
     * */
    std::vector<std::pair<int, int>> lit_len_code_lengths_to_write;
    // Lit/len
    int j = 0;
    while (j < 286)
    {
        int k = 1;
        while (j + k < 286 && lit_len_code_lengths[j + k] == lit_len_code_lengths[j] && k < 138)
            ++k;
        if (k >= 3 && lit_len_code_lengths[j] == 0)
        {
            if (j + k == 286)
                break; // Don't add the last 0s
            int code = k < 11 ? 17 : 18;
            code_lengths_frequency_table[code]++;
            lit_len_code_lengths_to_write.emplace_back(code, k);
        }
        else if (k >= 4)
        {
            // Code to repeat
            code_lengths_frequency_table[lit_len_code_lengths[j]]++;
            lit_len_code_lengths_to_write.emplace_back(lit_len_code_lengths[j], 1);
            // Repetition
            const int rep = k - 1;
            code_lengths_frequency_table[16] += rep / 6;
            for (int i = 0; i < rep / 6; ++i)
                lit_len_code_lengths_to_write.emplace_back(16, 6);
            int l = rep % 6;
            if (l > 2)
            {
                code_lengths_frequency_table[16]++;
                lit_len_code_lengths_to_write.emplace_back(16, l);
            }
            else
            {
                code_lengths_frequency_table[lit_len_code_lengths[j]] += l;
                for (int i = 0; i < l; ++i)
                    lit_len_code_lengths_to_write.emplace_back(lit_len_code_lengths[j], 1);
            }
        }
        else
        {
            code_lengths_frequency_table[lit_len_code_lengths[j]] += k;
            for (int i = 0; i < k; ++i)
                lit_len_code_lengths_to_write.emplace_back(lit_len_code_lengths[j], 1);
        }
        j += k;
    }
    const int provided_lit_len = j;
    // Distance
    /** \brief Code lengths to write <br>
     * 0-15 : raw length <br>
     * 16 : 3-6 repeat <br>
     * 17 : 3-10 0 <br>
     * 18 : 11-138 0 <br>
     * (code in [0, 18], length in [3, 138])
     * */
    std::vector<std::pair<int, int>> dist_code_lengths_to_write;
    j = 0;
    while (j < 30)
    {
        int k = 1;
        while (j + k < 30 && dist_code_lengths[j + k] == dist_code_lengths[j] && k < 30)
            ++k;
        if (k >= 3 && dist_code_lengths[j] == 0)
        {
            if (j + k == 30)
                break; // Don't add the last 0s
            int code = k < 11 ? 17 : 18;
            code_lengths_frequency_table[code]++;
            dist_code_lengths_to_write.emplace_back(code, k);
        }
        else if (k >= 4)
        {
            // Code to repeat
            code_lengths_frequency_table[dist_code_lengths[j]]++;
            dist_code_lengths_to_write.emplace_back(dist_code_lengths[j], 1);
            // Repetition
            const int rep = k - 1;
            code_lengths_frequency_table[16] += rep / 6;
            for (int i = 0; i < rep / 6; ++i)
                dist_code_lengths_to_write.emplace_back(16, 6);
            int l = rep % 6;
            if (l > 2)
            {
                code_lengths_frequency_table[16]++;
                dist_code_lengths_to_write.emplace_back(16, l);
            }
            else
            {
                code_lengths_frequency_table[dist_code_lengths[j]] += l;
                for (int i = 0; i < l; ++i)
                    dist_code_lengths_to_write.emplace_back(dist_code_lengths[j], 1);
            }
        }
        else
        {
            code_lengths_frequency_table[dist_code_lengths[j]] += k;
            for (int i = 0; i < k; ++i)
                dist_code_lengths_to_write.emplace_back(dist_code_lengths[j], 1);
        }
        j += k;
    }
    const int provided_dist_codes = j;
    Huffman_Tree code_lengths_tree(code_lengths_frequency_table);
    std::map<int, std::pair<int, int>> code_length_codes = code_lengths_tree.canonical_codes(7);
    code_length_codes_verif = code_length_codes;

    int num_code_length_code_length_to_write = 19;
    while (!code_length_codes.contains(code_length_codes_order[num_code_length_code_length_to_write - 1]) &&
           num_code_length_code_length_to_write >= 0)
        --num_code_length_code_length_to_write;

    //Writing to file
    Writer writer{};
    writer.write_number(1, 1); // BFINAL
    writer.write_number(2, 2); // BTYPE
    writer.write_number(provided_lit_len - 257, 5);
    writer.write_number(provided_dist_codes - 1, 5);
    writer.write_number(num_code_length_code_length_to_write - 4, 4);
    // Write the code length code lengths
    for (int k = 0; k < num_code_length_code_length_to_write; ++k)
        writer.write_number(code_length_codes[code_length_codes_order[k]].second, 3);
    int i = 0;
    // Write the literal/length code lengths
    for (const auto& [code_length, extra_bits_val] : lit_len_code_lengths_to_write)
    {
        writer.write_code(code_length_codes[code_length].first, code_length_codes[code_length].second);
        switch (code_length)
        {
            case 16:
                writer.write_number(extra_bits_val - 3, 2);
                i += extra_bits_val;
                break;
            case 17:
                writer.write_number(extra_bits_val - 3, 3);
                i += extra_bits_val;
                break;
            case 18:
                writer.write_number(extra_bits_val - 11, 7);
                i += extra_bits_val;
                break;
            default:
                i++;
                break;
        }
    }
    // Write the distance code lengths
    for (const auto& [code_length, extra_bits_val] : dist_code_lengths_to_write)
    {
        writer.write_code(code_length_codes[code_length].first, code_length_codes[code_length].second);
        switch (code_length)
        {
            case 16:
                writer.write_number(extra_bits_val - 3, 2);
                break;
            case 17:
                writer.write_number(extra_bits_val - 3, 3);
                break;
            case 18:
                writer.write_number(extra_bits_val - 11, 7);
                break;
            default:
                break;
        }
    }

    // Write the encoded data
    index = 0;
    nextMatchPos = matches.empty() ? size : matches.front()->position();
    nextMatchIter = matches.begin();
    while (index < size)
    {
        if (index == nextMatchPos)
        { // Write the match
            // Write the match
            const int length = (*nextMatchIter)->length();
            const int length_code_value = length_to_code(length);
            writer.write_code(lit_len_codes[length_code_value].first, lit_len_codes[length_code_value].second);
            // Write the extra bits
            const int extra_bits_value = length - lit_len_code_to_length[length_code_value];
            const int num_extra_bits = lengths_extra_bits[length_code_value];
            writer.write_number(extra_bits_value, num_extra_bits);

            // Write the distance
            const int distance = (*nextMatchIter)->distance();
            const int distance_code_value = distance_to_code(distance);
            writer.write_code(distance_codes[distance_code_value].first, distance_codes[distance_code_value].second);
            // Write the extra bits
            const int distance_extra_bits_value = distance - distance_code_to_distance[distance_code_value];
            const int num_distance_extra_bits = distance_extra_bits[distance_code_value];
            writer.write_number(distance_extra_bits_value, num_distance_extra_bits);

            index += (*nextMatchIter)->length();
            nextMatchPos = ++nextMatchIter == matches.end() ? size : (*nextMatchIter)->position();
        }
        else
        {
            writer.write_code(lit_len_codes[data[index]].first, lit_len_codes[data[index]].second);
            index++;
        }
    }

    writer.write_code(lit_len_codes[256].first, lit_len_codes[256].second); // End of block
    writer.close();

    // Copy the data to a new array
    auto* compressed_data = new unsigned char[writer.data.size()];
    for (int ind = 0; ind < writer.data.size(); ++ind)
        compressed_data[ind] = writer.data[ind];

    return {compressed_data, writer.data.size()};
}

std::pair<unsigned char*, int> Deflate::Main::inflate(const unsigned char* data, int offset)
{
    // Build static huffman tree if not already built
    if (static_huffman_tree_ == nullptr)
        build_static_huffman_tree();
    try
    {
        int off = offset;
        std::vector<std::vector<unsigned char>> inflated_blocks;
        Stream_Reader reader(data, offset);
        std::pair<bool, std::vector<unsigned char>> inflation;
        Window window{};
        do
        {
            inflation = decompress_block(reader, window);
            inflated_blocks.push_back(inflation.second);
            off += static_cast<int>(inflation.second.size());
        } while (!inflation.first);

        int inflated_size = 0;
        for (const auto& inflated_block : inflated_blocks)
            inflated_size += static_cast<int>(inflated_block.size());

        auto* inflated_data = new unsigned char[inflated_size];

        off = 0;
        for (const auto& inflatedBlock : inflated_blocks)
        {
            for (const unsigned char c : inflatedBlock)
                inflated_data[off++] = c;
        }

        return {inflated_data, inflated_size};
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
        throw;
    }
}

/// \brief Computes codes from code lengths using the <b>algorithm described in RFC 1951 section 3.2.2</b>
int* Deflate::Main::codes_from_code_lengths(const int code_lengths[], const int num_codes,
                                            const int max_code_length)
{
    // Compute number of codes for each code length
    /// \brief bl_count[i] contains the number of codes of length i
    int bl_count[max_code_length + 1];
    int* codes = new int[num_codes];
    for (int i = 0; i <= max_code_length; i++)
        bl_count[i] = 0;
    for (int i = 0; i < num_codes; ++i)
    {
        if (code_lengths[i] > 0)
            bl_count[code_lengths[i]]++;
    }

    // Compute the first code for each code length
    int next_code[max_code_length + 1];
    int code = 0;
    for (int bits = 1; bits <= max_code_length; bits++)
    {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    // Assign codes to each symbol
    for (int i = 0; i < num_codes; i++)
    {
        const int code_length = code_lengths[i];
        if (code_length > 0)
            codes[i] = next_code[code_length]++;
        else codes[i] = -1;
    }

    return codes;
}

std::vector<unsigned char> Deflate::Main::read_dynamic_huffman_data(Stream_Reader& reader, Huffman_Tree& lit_len_tree,
                                                                    Huffman_Tree& dist_tree, Window& window)
{
    std::vector<unsigned char> data;
    int lit_len = lit_len_tree.read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255)
        {
            data.push_back(static_cast<char>(lit_len));
            window.add(static_cast<char>(lit_len));
        }
        else if (lit_len <= 287) // Length
        {
            int len = lit_len_code_to_length[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len])
                len += reader.read_number(extra_bits);
            // Distance
            const int dist_value = dist_tree.read_key(reader);
            if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");
            int dist = distance_code_to_distance[dist_value];
            if (const int extra_bits = distance_extra_bits[dist_value])
                dist += reader.read_number(extra_bits);

            for (int i = 0; i < len; ++i)
            {
                char c = window.get(dist);
                data.push_back(c);
                window.add(c);
            }
        }
        else
            throw std::runtime_error("Invalid dynamic huffman code!");

        lit_len = lit_len_tree.read_key(reader);
    }

    return data;
}

std::list<Deflate::Match*> Deflate::Main::find_matches(const unsigned char* data, int size, int offset)
{
    std::list<Match*> matches;
    std::unordered_map<int, std::list<int>> q;
    bool matchOnPreviousByte = false;
    int ind = offset;
    while (ind < size - 2)
    {
        const int h = data[ind] << 16 | data[ind + 1] << 8 | data[ind + 2];
        bool match = q.contains(h);
        if (!match)
            q[h] = {ind++};
        else
        {
            auto* bestMatch = new Match(-1, -1, -1);
            for (auto j : q[h])
            {
                if (ind - j > 32768) continue;
                int len = 3;
                while (ind + len < size && data[j + len] == data[ind + len] && len < 258)
                    ++len;
                if (len > bestMatch->length())
                {
                    delete bestMatch;
                    bestMatch = new Match(ind, len, ind - j);
                }
            }
            // All matches are too far away
            if (bestMatch->length() == -1)
            {
                q[h].emplace_back(ind++);
                continue;
            }
            // Lazy matching
            if (matchOnPreviousByte && bestMatch->length() > matches.back()->length())
            {
                delete matches.back();
                matches.pop_back();
            }
            matches.push_back(bestMatch);
            q[h].push_back(ind);
            // Add the hashes for the bytes in the match
            for (int i = 1; i < bestMatch->length(); ++i)
            {
                ind++;
                const int h2 = data[ind] << 16 | data[ind + 1] << 8 | data[ind + 2];
                q[h2].push_back(ind);
            }
            ind++;
        }
        matchOnPreviousByte = match;
    }

    return matches;
}

int Deflate::Main::length_to_code(const int length)
{
    int prev = 257;
    for (const auto& [len, code] : length_codes)
    {
        if (length < len)
            return prev;
        prev = code;
    }
    return prev; // 285
}

int Deflate::Main::distance_to_code(const int distance)
{
    int prev = 1;
    for (const auto& [dist, code] : dist_codes)
    {
        if (distance < dist)
            return prev;
        prev = code;
    }
    return prev; // 30
}

std::string beautiful_size_display(const std::vector<unsigned char>::size_type size_in_byte)
{
    std::string size;
    if (size_in_byte < 1024)
        size = std::to_string(size_in_byte) + " B";
    else if (size_in_byte < 1024 * 1024)
        size = std::to_string(size_in_byte / 1024) + " KB";
    else if (size_in_byte < 1024 * 1024 * 1024)
        size = std::to_string(size_in_byte / (1024 * 1024)) + " MB";
    else
        size = std::to_string(size_in_byte / (1024 * 1024 * 1024)) + " GB";

    return size;
}

void Deflate::Main::Test()
{
    std::cout << "Testing Calgary Corpus\n";
    std::cout << std::left << std::setw(20) << "File" << std::setw(15) << "Original" << std::setw(15) << "Compressed"
              << std::setw(15) << "Ratio(%)" << std::setw(15) << "Time(ms)" << std::endl;

    std::ifstream in;
    for (const auto& file_name : std::filesystem::directory_iterator("../Data/calgaryCorpus"))
    {
        /*if (file_name.path() != "../Data/calgaryCorpus\\pic")
            continue;*/
        in.open(file_name.path(), std::ios::binary);
        std::cout << std::left << std::setw(20) << file_name.path().filename().string();

        if (!in.is_open())
            throw std::runtime_error("Could not open file!");

        // Read the file
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::cout << std::left << std::setw(15) << beautiful_size_display(data.size());
        in.close();

        // Compress the file block by block
        auto start = std::chrono::steady_clock::now();
        int compressed_size = 0;
        for (int i = 0; i < static_cast<int>(data.size()); i += BLOCK_SIZE)
        {
            const int data_size = std::min(BLOCK_SIZE, static_cast<int>(data.size()) - i);
            std::pair<unsigned char*, int> compressed = deflate((data.data() + i), data_size);
            //std::pair<unsigned char*, int> decompressed = inflate(compressed.first);
            compressed_size += compressed.second;

            /*for (int j = 0; j < data_size; ++j)
            {
                if (data[i + j] != decompressed.first[j])
                    std::cout << "Error at index " << j << "!" << std::endl;
            }*/
        }
        auto end = std::chrono::steady_clock::now();
        std::cout << std::left << std::setw(15) << beautiful_size_display(compressed_size);
        std::cout << std::left << std::setw(15) << std::fixed << std::setprecision(2)
                  << round((1 - (static_cast<double>(compressed_size) / static_cast<double>(data.size()))) * 10000) /
                     100;
        std::cout << std::left << std::setw(15)
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    }
}
