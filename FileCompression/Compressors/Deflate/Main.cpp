#include "Main.h"
#include "Huffman_Tree.h"
#include "Window.h"
#include "Match.h"
#include <exception>
#include <iostream>
#include <vector>
#include <list>

std::pair<bool, std::vector<char>> Deflate::Main::decompress_block(Stream_Reader& reader, Window& window)
{
    const bool is_final_block = reader.read_bits(1)[0];
    
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

std::vector<char> Deflate::Main::get_stored_data(Stream_Reader& reader)
{
    reader.skip_end_of_byte();
    const auto len = static_cast<short>(reader.read_number(16));
    const auto nlen = static_cast<short>(reader.read_number(16));
    if (len != ~nlen)
        throw std::runtime_error("Invalid stored block length!");
    std::vector<char> bytes = reader.read_bytes_v(len);

    return bytes;
}

std::vector<char> Deflate::Main::get_fixed_huffman_data(Stream_Reader& reader, Window& window)
{
    std::vector<char> data;
    int lit_len = static_huffman_tree_->read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255) {
            data.push_back(static_cast<char>(lit_len));
            window.add(static_cast<char>(lit_len));
        }
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
            for (int i = 0; i < len; ++i) {
                char c = window.get(dist); // Current pos increases when we add to window the offset is always dist
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

std::vector<char> Deflate::Main::get_dynamic_huffman_data(Stream_Reader& reader, Window& window)
{
    std::vector<char> data;
    const int num_lit_len = reader.read_number(5) + 257;
    const int num_dist = reader.read_number(5) + 1;
    const int num_code_length_code_length = reader.read_number(4) + 4;
    int code_length_code_lengths[19];

    // Read code length code lengths
    int max_code_length_code_length = 0;
    for (int i = 0; i < num_code_length_code_length; ++i) {
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
    for (int i = 0; i < 19; ++i) {
        if (code_length_codes[i] != -1)
            code_length_keys_paths[i] = {code_length_codes[i], code_length_code_lengths[i]};
    }
    Huffman_Tree code_lengths_tree(code_length_keys_paths);
    delete code_length_codes;

    // Read lit_len code lengths
    int read_codes = 0;
    int lit_len_code_lengths[num_lit_len];
    int max_lit_len_code_length = 0;
    while (read_codes < num_lit_len){
        const int code = code_lengths_tree.read_key(reader);
        if (code < 16)
            lit_len_code_lengths[read_codes++] = code;
        else if (code == 16) {
            const int repeat = reader.read_number(2) + 3;
            for (int i = 0; i < repeat; ++i)
                lit_len_code_lengths[read_codes++] = lit_len_code_lengths[read_codes - 1];
        }
        else if (code == 17) {
            const int repeat = reader.read_number(3) + 3;
            for (int i = 0; i < repeat; ++i)
                lit_len_code_lengths[read_codes++] = 0;
        }
        else if (code == 18) {
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
    for (int i = 0; i < num_lit_len; ++i) {
        if (lit_len_codes[i] != -1)
            lit_len_codes_keys_paths[i] = {lit_len_codes[i], lit_len_code_lengths[i]};
    }
    Huffman_Tree lit_len_tree(lit_len_codes_keys_paths);
    delete lit_len_codes;

    // Read dist code lengths
    read_codes = 0;
    int dist_code_lengths[num_dist];
    int max_dist_code_length = 0;
    while (read_codes < num_dist){
        const int code = code_lengths_tree.read_key(reader);
        if (code < 16)
            dist_code_lengths[read_codes++] = code;
        else if (code == 16) {
            const int repeat = reader.read_number(2) + 3;
            for (int i = 0; i < repeat; ++i)
                dist_code_lengths[read_codes++] = dist_code_lengths[read_codes - 1];
        }
        else if (code == 17) {
            const int repeat = reader.read_number(3) + 3;
            for (int i = 0; i < repeat; ++i)
                dist_code_lengths[read_codes++] = 0;
        }
        else if (code == 18) {
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
    int* dist_codes = codes_from_code_lengths(dist_code_lengths, num_dist, max_dist_code_length);
    std::map<int, std::pair<int, int>> dist_codes_keys_paths;
    for (int i = 0; i < num_dist; ++i) {
        if (dist_codes[i] != -1)
            dist_codes_keys_paths[i] = {dist_codes[i], dist_code_lengths[i]};
    }
    Huffman_Tree dist_tree(dist_codes_keys_paths);
    delete dist_codes;

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

std::pair<char*, int> Deflate::Main::deflate(const char *data, int size)
{
    // Spot matches
    std::list<Match*> matches;
    std::map<int, std::list<int>> q;
    bool matchOnPreviousByte = false;
    int i = 0;
    while (i < size - 2) {
        const int h = data[i] << 16 | data[i + 1] << 8 | data[i + 2];
        bool match = q.find(h) != q.end();
        if (!match)
            q[h] = {i++};
        else {
            auto* bestMatch = new Match(i, 3, 3);
            for (auto j : q[h]) {
                if (i - j > 32768) continue;
                int len = 3;
                while (i + len < size && data[j + len] == data[i + len] && len < 258)
                    ++len;
                if (len > bestMatch->length())
                    bestMatch = new Match(i, len, i - j);
            }
            // Lazy matching
            if (matchOnPreviousByte && bestMatch > matches.back()) {
                delete matches.back();
                matches.pop_back();
                i++;
            }
            matches.push_back(bestMatch);
            q[h].push_back(i);
            i += bestMatch->length();
        }
        matchOnPreviousByte = match;
    }

    // Build dynamic huffman trees
    std::unordered_map<int, int> lit_len_frequency_table;
    std::unordered_map<int, int> dist_frequency_table;
    int index = 0;
    int nextMatchPos = !matches.empty() ? matches.front()->position() : size;
    auto nextMatchIter = matches.begin();
    while (index < size){
        // ??
        /*if (data[index] == 0) {
            int j = index + 1;
            while (j < size && data[j] == 0 && j - index < 138)
                ++j;
            if (j - index >= 3) {
                matches.push_back(new Match(index, 0, j - index));
                index = j;
            }
        }
        else
            ++index;*/
        if (index == nextMatchPos){
            int length_code = length_codes[(*nextMatchIter)->length()];
            int dist_code = dist_codes[(*nextMatchIter)->distance()];

            if (lit_len_frequency_table.find(length_code) == lit_len_frequency_table.end())
                lit_len_frequency_table[length_code] = 1;
            else
                lit_len_frequency_table[length_code]++;

            if (lit_len_frequency_table.find(dist_code) == lit_len_frequency_table.end())
                dist_frequency_table[dist_code] = 1;
            else
                dist_frequency_table[dist_code]++;

            index += (*nextMatchIter)->length();
            nextMatchPos = ++nextMatchIter == matches.end() ? size : (*nextMatchIter)->position();
        }
        else {
            if (lit_len_frequency_table.find(data[index]) == lit_len_frequency_table.end())
                lit_len_frequency_table[data[index]] = 1;
            else
                lit_len_frequency_table[data[index]]++;
            index++;
        }
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
    for (int & lit_len_code_length : lit_len_code_lengths)
        lit_len_code_length = 0;
    for (int & dist_code_length : dist_code_lengths)
        dist_code_length = 0;
    for (const auto& [c, len] : lit_len_tree.code_lengths())
        lit_len_code_lengths[c] = len;
    for (const auto& [c, len] : dist_tree.code_lengths())
        dist_code_lengths[c] = len;
    std::unordered_map<int, int> code_lengths_frequency_table;
    int j = 0;
    while (j < 286){
        int k = 1;
        while (j + k < 286 && lit_len_code_lengths[j + k] == lit_len_code_lengths[j] && k < 138)
            ++k;
        if (k > 2) {
            if (lit_len_code_lengths[j] == 0)
                code_lengths_frequency_table[k < 11 ? 17 : 18]++;
            else
            {
                code_lengths_frequency_table[16] += k / 6;
                int l = k % 6;
                if (l > 2)
                    code_lengths_frequency_table[16]++;
                else
                    code_lengths_frequency_table[lit_len_code_lengths[j]] += l;
            }
        }
        else
            code_lengths_frequency_table[lit_len_code_lengths[j]] += k;
        j += k;
    }
    Huffman_Tree code_lengths_tree(code_lengths_frequency_table);

    std::cout<<std::endl;
    throw std::runtime_error("Not implemented!");
    char* res = new char[size + 1];
    res[0] = 1; // Last block, no compression
    for (int i = 0; i < size; i++)
        res[i + 1] = static_cast<char>(data[i]);

    return {res, size + 1};
}

std::pair<char*, int> Deflate::Main::inflate(const unsigned char* data, const int offset)
{
    // Build static huffman tree if not already built
    if (static_huffman_tree_ == nullptr)
        build_static_huffman_tree();
    try
    {
        int off = offset;
        std::vector<std::vector<char>> inflated_blocks;
        Stream_Reader reader(data, offset);
        std::pair<bool, std::vector<char>> inflation;
        Window window{};
        do
        {
            inflation = decompress_block(reader, window);
            inflated_blocks.push_back(inflation.second);
            off += static_cast<int>(inflation.second.size());
        }
        while (!inflation.first);
        
        int inflated_size = 0;
        for (const auto& inflated_block : inflated_blocks)
            inflated_size += static_cast<int>(inflated_block.size());

        char* inflated_data = new char[inflated_size];

        off = 0;
        for (const auto& inflatedBlock : inflated_blocks)
        {
            for (const char c : inflatedBlock)
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
                                       const int max_code_length) {
    // Compute number of codes for each code length
    /// \brief bl_count[i] contains the number of codes of length i
    int bl_count[max_code_length + 1];
    int* code_length_codes = new int[num_codes];
    for (int i = 0; i <= max_code_length; i++)
        bl_count[i] = 0;
    for (int i = 0; i < num_codes; ++i) {
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
        const int code_length_code_length = code_lengths[i];
        if (code_length_code_length > 0)
            code_length_codes[i] = next_code[code_length_code_length]++;
        else code_length_codes[i] = -1;
    }

    return code_length_codes;
}

std::vector<char> Deflate::Main::read_dynamic_huffman_data(Stream_Reader &reader, Huffman_Tree &lit_len_tree,
                                                     Huffman_Tree &dist_tree, Window& window) {
    std::vector<char> data;
    int lit_len = lit_len_tree.read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255) {
            data.push_back(static_cast<char>(lit_len));
            window.add(static_cast<char>(lit_len));
        }
        else if (lit_len <= 287) // Length
        {
            int len = lengths_base_values_[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len])
                len += reader.read_number(extra_bits);
            // Distance
            const int dist_value = dist_tree.read_key(reader);
            if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");
            int dist = distances_base_values[dist_value];
            if (const int extra_bits = distances_extra_bits[dist_value])
                dist += reader.read_number(extra_bits);

            for (int i = 0; i < len; ++i) {
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
