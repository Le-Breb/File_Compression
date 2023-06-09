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
            data.push_back(static_cast<unsigned char>(lit_len));
            window.add(static_cast<unsigned char>(lit_len));
        }
        else if (lit_len <= 287) // Repetition
        {
            // Length
            int len = lit_len_code_to_length[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len]) // Extra bits
                len += reader.read_number(extra_bits);
            // Distance
            const int dist_value = reader.read_number(5);
            if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");
            int dist = distance_code_to_distance[dist_value];
            if (const int extra_bits = distance_extra_bits[dist_value]) // Extra bits
                dist += reader.read_number(extra_bits);
            for (int i = 0; i < len; ++i)
            {
                unsigned char c = window.get(
                        dist); // Current pos increases when we add to window so the offset is always dist
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

std::vector<unsigned char> Deflate::Main::get_dynamic_huffman_data(Stream_Reader& reader, Window& window)
{
    std::vector<unsigned char> data;

    // Read the number of different codes to read
    const int num_lit_len = reader.read_number(5) + 257;
    const int num_dist = reader.read_number(5) + 1;
    const int num_code_length_code_length = reader.read_number(4) + 4;

    // Read code length code lengths
    int max_code_length_code_length = 0;
    int code_length_code_lengths[19];
    for (int i = 0; i < num_code_length_code_length; ++i)
    {
        int code_length_code_length = reader.read_number(3);
        code_length_code_lengths[code_length_codes_order[i]] = code_length_code_length;
        if (code_length_code_length > max_code_length_code_length)
            max_code_length_code_length = code_length_code_length;
    }
    for (int i = num_code_length_code_length; i < 19; ++i)
        code_length_code_lengths[code_length_codes_order[i]] = 0;

    Huffman_Tree* code_lengths_tree = code_lengths_to_tree(code_length_code_lengths, 19, max_code_length_code_length);

    // Read lit_len code lengths
    int lit_len_code_lengths[num_lit_len];
    int max_lit_len_code_length = 0;
    read_code_lengths(reader, code_lengths_tree, num_lit_len, max_lit_len_code_length, lit_len_code_lengths);

    // Build lit_len tree with the code lengths
    Huffman_Tree* lit_len_tree = code_lengths_to_tree(lit_len_code_lengths, num_lit_len, max_lit_len_code_length);

    // Read dist code lengths
    int dist_code_lengths[num_dist];
    int max_dist_code_length = 0;
    read_code_lengths(reader, code_lengths_tree, num_dist, max_dist_code_length, dist_code_lengths);

    // Build dist tree with the code lengths
    Huffman_Tree* dist_tree = code_lengths_to_tree(dist_code_lengths, num_dist, max_dist_code_length);

    // Read the data
    std::vector<unsigned char> decompressed_data = read_dynamic_huffman_data(reader, lit_len_tree, dist_tree, window);

    // Free memory
    delete lit_len_tree;
    delete dist_tree;
    delete code_lengths_tree;

    return decompressed_data;
}

void Deflate::Main::build_static_huffman_tree()
{
    std::map<int, Deflate::Huffman_Tree::Code> keys_paths;
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

    // Compute the trees
    Huffman_Tree* lit_len_tree;
    Huffman_Tree* dist_tree;
    compute_dynamic_trees(data, size, matches, lit_len_tree, dist_tree);

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
    std::map<int, Deflate::Huffman_Tree::Code> lit_len_codes = lit_len_tree->canonical_codes(MAX_CODE_LENGTH);
    std::map<int, Deflate::Huffman_Tree::Code> distance_codes = dist_tree->canonical_codes(MAX_CODE_LENGTH);
    for (int& lit_len_code_length : lit_len_code_lengths)
        lit_len_code_length = 0;
    for (int& dist_code_length : dist_code_lengths)
        dist_code_length = 0;
    for (const auto& [c, code_len] : lit_len_codes)
        lit_len_code_lengths[c] = code_len.length;
    for (const auto& [c, code_len] : distance_codes)
        dist_code_lengths[c] = code_len.length;
    std::unordered_map<int, int> code_lengths_frequency_table;

    // Lit/len
    std::vector<std::pair<int, int>> lit_len_code_lengths_to_write;
    const int provided_lit_len = enumerate_code_lengths(286, lit_len_code_lengths, 138,
                                                        lit_len_code_lengths_to_write, code_lengths_frequency_table);

    // Distance
    std::vector<std::pair<int, int>> dist_code_lengths_to_write;
    const int provided_dist_codes = enumerate_code_lengths(30, dist_code_lengths, 30,
                                                           dist_code_lengths_to_write, code_lengths_frequency_table);

    // Code length code lengths
    Huffman_Tree code_lengths_tree(code_lengths_frequency_table);
    std::map<int, Deflate::Huffman_Tree::Code> code_length_codes = code_lengths_tree.canonical_codes(
            MAX_CODE_LENGTH_CODE_LENGTH);

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
        writer.write_number(code_length_codes[code_length_codes_order[k]].length, 3);

    write_code_lengths(writer, lit_len_code_lengths_to_write, code_length_codes); // Write lit/len code lengths
    write_code_lengths(writer, dist_code_lengths_to_write, code_length_codes); // Write distance code lengths

    write_compressed_data(writer, data, size, matches, lit_len_codes, distance_codes); // Write compressed data

    writer.write_code(lit_len_codes[256].code, lit_len_codes[256].length); // Write end of block
    writer.close();

    // Copy the data to a new array
    auto* compressed_data = new unsigned char[writer.data.size()];
    for (int ind = 0; ind < writer.data.size(); ++ind)
        compressed_data[ind] = writer.data[ind];

    delete lit_len_tree;
    delete dist_tree;

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
int* Deflate::Main::codes_from_code_lengths(const int code_lengths[], const int num_symbols,
                                            const int max_code_length)
{
    // Compute number of codes for each code length
    /// \brief bl_count[i] contains the number of codes of length i
    int bl_count[max_code_length + 1];
    int* codes = new int[num_symbols];
    for (int i = 0; i <= max_code_length; i++)
        bl_count[i] = 0;
    for (int i = 0; i < num_symbols; ++i)
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
    for (int i = 0; i < num_symbols; i++)
    {
        const int code_length = code_lengths[i];
        if (code_length > 0)
            codes[i] = next_code[code_length]++;
        else codes[i] = -1;
    }

    return codes;
}

std::vector<unsigned char> Deflate::Main::read_dynamic_huffman_data(Stream_Reader& reader, Huffman_Tree* lit_len_tree,
                                                                    Huffman_Tree* dist_tree, Window& window)
{
    std::vector<unsigned char> data;
    int lit_len = lit_len_tree->read_key(reader);
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
            const int dist_value = dist_tree->read_key(reader);
            if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");
            int dist = distance_code_to_distance[dist_value];
            if (const int extra_bits = distance_extra_bits[dist_value])
                dist += reader.read_number(extra_bits);

            for (int i = 0; i < len; ++i)
            {
                unsigned char c = window.get(dist);
                data.push_back(c);
                window.add(c);
            }
        }
        else
            throw std::runtime_error("Invalid dynamic huffman code!");

        lit_len = lit_len_tree->read_key(reader);
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
            std::pair<unsigned char*, int> decompressed = inflate(compressed.first);
            compressed_size += compressed.second;

            for (int j = 0; j < data_size; ++j)
            {
                if (data[i + j] != decompressed.first[j])
                    std::cout << "Error at index " << j << "!" << std::endl;
            }
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

Deflate::Huffman_Tree*
Deflate::Main::code_lengths_to_tree(int* code_lengths, const int num_symbols, const int max_length)
{
    int* code_length_codes = codes_from_code_lengths(code_lengths, num_symbols, max_length);
    std::map<int, Deflate::Huffman_Tree::Code> code_length_keys_paths;
    for (int i = 0; i < num_symbols; ++i)
    {
        if (code_length_codes[i] != -1)
            code_length_keys_paths[i] = {code_length_codes[i], code_lengths[i]};
    }
    delete code_length_codes;

    return new Huffman_Tree(code_length_keys_paths);
}

void
Deflate::Main::read_code_lengths(Deflate::Stream_Reader& reader, const Deflate::Huffman_Tree* tree,
                                 const int num_symbols,
                                 int& max_length, int* code_lengths)
{
    int read_codes = 0;
    while (read_codes < num_symbols)
    {
        const int code = tree->read_key(reader);
        switch (code)
        {
            case 16: // Repeat last code 3-6 times
            {
                const int repeat = reader.read_number(2) + 3;
                for (int i = 0; i < repeat; ++i)
                    code_lengths[read_codes++] = code_lengths[read_codes - 1];
                break;
            }
            case 17: // Repeat 0 3-10 times
            {
                const int repeat = reader.read_number(3) + 3;
                for (int i = 0; i < repeat; ++i)
                    code_lengths[read_codes++] = 0;
                break;
            }
            case 18: // Repeat 0 11-138 times
            {
                const int repeat = reader.read_number(7) + 11;
                for (int i = 0; i < repeat; ++i)
                    code_lengths[read_codes++] = 0;
                break;
            }
            default: // Normal code length
                if (code < 0 || code > 15)
                    throw std::runtime_error("Invalid code length code!");
                code_lengths[read_codes++] = code;
                break;
        }
        // Update max length
        if (code_lengths[read_codes - 1] > max_length)
            max_length = code_lengths[read_codes - 1];
    }
}

void Deflate::Main::compute_dynamic_trees(const unsigned char* data, const int size, std::list<Match*>& matches,
                                          Deflate::Huffman_Tree*& lit_len_tree,
                                          Deflate::Huffman_Tree*& dist_tree)
{
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

    lit_len_tree = new Huffman_Tree(lit_len_frequency_table);
    dist_tree = new Huffman_Tree(dist_frequency_table);
}

int Deflate::Main::enumerate_code_lengths(int count, const int* code_lengths, const int max_repetition,
                                          std::vector<std::pair<int, int>>& code_lengths_to_write,
                                          std::unordered_map<int, int>& code_lengths_frequency_table)
{
    int j = 0;
    while (j < count)
    {
        int k = 1;
        while (j + k < count && code_lengths[j + k] == code_lengths[j] && k < max_repetition)
            ++k;
        if (k >= 3 && code_lengths[j] == 0)
        {
            if (j + k == count)
                break; // Don't add the last 0s
            int code = k < 11 ? 17 : 18;
            code_lengths_frequency_table[code]++;
            code_lengths_to_write.emplace_back(code, k);
        }
        else if (k >= 4)
        {
            // Code to repeat
            code_lengths_frequency_table[code_lengths[j]]++;
            code_lengths_to_write.emplace_back(code_lengths[j], 1);
            // Repetition
            const int rep = k - 1;
            code_lengths_frequency_table[16] += rep / 6;
            for (int i = 0; i < rep / 6; ++i)
                code_lengths_to_write.emplace_back(16, 6);
            int l = rep % 6;
            if (l > 2)
            {
                code_lengths_frequency_table[16]++;
                code_lengths_to_write.emplace_back(16, l);
            }
            else
            {
                code_lengths_frequency_table[code_lengths[j]] += l;
                for (int i = 0; i < l; ++i)
                    code_lengths_to_write.emplace_back(code_lengths[j], 1);
            }
        }
        else
        {
            code_lengths_frequency_table[code_lengths[j]] += k;
            for (int i = 0; i < k; ++i)
                code_lengths_to_write.emplace_back(code_lengths[j], 1);
        }
        j += k;
    }


    return j;
}

void Deflate::Main::write_code_lengths(Deflate::Writer& writer, std::vector<std::pair<int, int>>& code_lengths,
                                       std::map<int, Deflate::Huffman_Tree::Code>& code_length_codes)
{
    for (const auto& [code_length, extra_bits_val] : code_lengths)
    {
        writer.write_code(code_length_codes[code_length].code, code_length_codes[code_length].length);
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
}

void Deflate::Main::write_compressed_data(Deflate::Writer& writer, const unsigned char* data, const int size,
                                          std::list<Match*>& matches,
                                          std::map<int, Deflate::Huffman_Tree::Code>& lit_len_codes,
                                          std::map<int, Deflate::Huffman_Tree::Code>& distance_codes)
{
    int index = 0;
    int nextMatchPos = matches.empty() ? size : matches.front()->position();
    auto nextMatchIter = matches.begin();
    while (index < size)
    {
        if (index == nextMatchPos)
        {
            // Write the match
            const int length = (*nextMatchIter)->length();
            const int length_code_value = length_to_code(length);
            writer.write_code(lit_len_codes[length_code_value].code, lit_len_codes[length_code_value].length);
            // Write the extra bits
            const int extra_bits_value = length - lit_len_code_to_length[length_code_value];
            const int num_extra_bits = lengths_extra_bits[length_code_value];
            writer.write_number(extra_bits_value, num_extra_bits);

            // Write the distance
            const int distance = (*nextMatchIter)->distance();
            const int distance_code_value = distance_to_code(distance);
            writer.write_code(distance_codes[distance_code_value].code, distance_codes[distance_code_value].length);
            // Write the extra bits
            const int distance_extra_bits_value = distance - distance_code_to_distance[distance_code_value];
            const int num_distance_extra_bits = distance_extra_bits[distance_code_value];
            writer.write_number(distance_extra_bits_value, num_distance_extra_bits);

            index += (*nextMatchIter)->length();
            nextMatchPos = ++nextMatchIter == matches.end() ? size : (*nextMatchIter)->position();
        }
        else
        {
            writer.write_code(lit_len_codes[data[index]].code, lit_len_codes[data[index]].length);
            index++;
        }
    }
}
