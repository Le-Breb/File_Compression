#include "Main.h"
#include "Huffman_Tree.h"
#include "Window.h"
#include "Match.h"
#include "Writer.h"
#include "Memory.h"
#include <exception>
#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <iomanip>
#include <bitset>

bool
Deflate::Main::decompress_block(Stream_Reader& reader, Window& window, Writer& writer)
{
    const bool is_final_block = reader.read_bit();

    switch (reader.read_number(2))
    {
        case 0:
            get_stored_data(reader, writer);
            break;
        case 1:
            get_fixed_huffman_data(reader, window, writer);
            break;
        case 2:
            get_dynamic_huffman_data(reader, window, writer);
            break;
        case 3:
            throw std::runtime_error("Reserved block type!");
        default: // Should never happen
            throw std::runtime_error("Unknown block type!");
    }

    return is_final_block;
}

void Deflate::Main::get_stored_data(Stream_Reader& reader, Writer& writer)
{
    reader.skip_end_of_byte(); // Padding

    // Read data length
    const auto len = static_cast<short>(reader.read_number(16));
    const auto nlen = static_cast<short>(reader.read_number(16));
    if (len != ~nlen)
        throw std::runtime_error("Invalid stored block length!");

    // Read data
    for (const Byte& b : reader.read_bytes_v(len))
        writer.write_raw_byte(b);
}

void Deflate::Main::get_fixed_huffman_data(Stream_Reader& reader, Window& window, Writer& writer)
{
    int lit_len = static_huffman_tree_->read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255)
        {
            writer.write_raw_byte(static_cast<Byte>(lit_len));
            window.add(static_cast<Byte>(lit_len));
        }
        else if (lit_len <= 287) // Repetition
        {
            // Length
            int len = lit_len_code_to_length[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len]) // Extra bits
                len += reader.read_number(extra_bits);

            // Distance
            const int dist_value = reader.read_number(5);
            /*if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");*/
            int dist = dist_code_to_dist[dist_value];
            if (const int extra_bits = distances_extra_bits[dist_value]) // Extra bits
                dist += reader.read_number(extra_bits);

            // Read match
            for (int i = 0; i < len; ++i)
            {
                // Current pos increases when we add to window so the offset is always dist
                const Byte c = window.get(dist);
                writer.write_raw_byte(c);
                window.add(c);
            }
        }
        else
            throw std::runtime_error("Invalid fixed huffman code!");

        lit_len = static_huffman_tree_->read_key(reader);
    }
}

void Deflate::Main::get_dynamic_huffman_data(Stream_Reader& reader, Window& window, Writer& writer)
{
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

    // Build code length tree with the lengths of the code length codes
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
    read_dynamic_huffman_data(reader, lit_len_tree, dist_tree, window, writer);

    // Free memory
    delete lit_len_tree;
    delete dist_tree;
    delete code_lengths_tree;
}

void Deflate::Main::build_fixed_huffman_tree()
{
    auto* keys_paths = new Deflate::Huffman_Tree::Code[288];
    int c = 0;
    for (int i = 0b00110000; i <= 0b10111111; i++)
        keys_paths[c++] = {i, 8};
    for (int i = 0b110010000; i <= 0b111111111; i++)
        keys_paths[c++] = {i, 9};
    for (int i = 0b0000000; i <= 0b0010111; i++)
        keys_paths[c++] = {i, 7};
    for (int i = 0b11000000; i <= 0b11000111; i++)
        keys_paths[c++] = {i, 8};

    static_huffman_tree_ = new Huffman_Tree(keys_paths, 288);
}

std::vector<Byte> Deflate::Main::deflate(const Byte* data, int size)
{
    // Build fixed huffman lit len values codes if not done yet
    if (fixed_lit_len_values_codes.empty())
        build_fixed_huffman_lit_len_values_codes();

    int ind = 0;
    std::vector<Byte> compressed_data;
    Writer writer(&compressed_data);
    Memory mem{};
    // While there is still data to compress
    while (ind < size)
    {
        // Compute dynamic compression data
        const dynamic_comp_res dynamic_comp_res = compute_dynamic_comp_data(data, size, writer, ind, mem);
        // Compute compressed size with dynamic and fixed codes
        const int dynamic_compressed_size = compressed_size_with_dynamic_codes(data, dynamic_comp_res);
        const int fixed_compressed_size = compressed_size_with_fixed_codes(data, dynamic_comp_res);

        // Select best compression mode
        // Dynamic codes are only used if they are smaller than fixed codes and the uncompressed data
        if (dynamic_compressed_size >= fixed_compressed_size ||
            dynamic_compressed_size >= dynamic_comp_res.uncompressed_size)
        {
            // Use fixed codes if they are smaller than the uncompressed data
            if (fixed_compressed_size >= dynamic_comp_res.uncompressed_size)
                deflate_uncompressed(data, dynamic_comp_res.uncompressed_size, ind,
                                     dynamic_comp_res.is_last_block, writer);

            else // No compression if both static and dynamic do not reduce the size
                deflate_fixed(data, writer, dynamic_comp_res, fixed_compressed_size);
        }

        else deflate_dynamic(dynamic_comp_res, writer, data, dynamic_compressed_size);

        ind += dynamic_comp_res.uncompressed_size;
        mem.Clean();
    }

    writer.close(); // Close the writer to write the last byte
    return compressed_data;
}

std::vector<Byte> Deflate::Main::inflate(const std::vector<Byte> data)
{
    // Build static huffman data if not done yet
    if (static_huffman_tree_ == nullptr)
        build_fixed_huffman_tree();
    try
    {
        std::vector<Byte> inflated_data;
        Writer writer(&inflated_data);
        Stream_Reader reader(&data);
        Window window{};
        do
        {
        } while (!decompress_block(reader, window, writer)); // Decompress blocks until the last block is reached

        return inflated_data;
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
        throw;
    }
}

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

void
Deflate::Main::read_dynamic_huffman_data(Stream_Reader& reader, const Huffman_Tree* lit_len_tree,
                                         const Huffman_Tree* dist_tree,
                                         Window& window,
                                         Writer& writer)
{
    int lit_len = lit_len_tree->read_key(reader);
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255)
        {
            writer.write_raw_byte(static_cast<char>(lit_len));
            window.add(static_cast<char>(lit_len));
        }
        else if (lit_len <= 287) // Length
        {
            int len = lit_len_code_to_length[lit_len];
            if (const int extra_bits = lengths_extra_bits[lit_len])
                len += reader.read_number(extra_bits);

            // Distance
            const int dist_value = dist_tree->read_key(reader);
            /*if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");*/
            int dist = dist_code_to_dist[dist_value];
            if (const int extra_bits = distances_extra_bits[dist_value])
                dist += reader.read_number(extra_bits);

            // Read match
            for (int i = 0; i < len; ++i)
            {
                Byte c = window.get(dist);
                writer.write_raw_byte(c);
                window.add(c);
            }
        }
        else
            throw std::runtime_error("Invalid dynamic huffman code!");

        lit_len = lit_len_tree->read_key(reader);
    }
}

int Deflate::Main::compute_dynamic_trees(const Byte* data, const int offset, const int size, std::list<Match>& matches,
                                         Huffman_Tree*& lit_len_tree, Huffman_Tree*& dist_tree,
                                         Memory& mem)
{
    bool matchOnPreviousByte = false;
    int ind = offset;
    mem.lit_len_frequency_table[256] = 1; // End of block
    int num_symbols = 1; // End of block
    int h = data[ind];

    int prev_match = -1;
    int prev_match_len = 0;
    int prev_match_dist = 0;

    update_hash(h, data[ind + 1]); // Update hash of first two bytes

    // Find matches
    while (ind < size - 2 && num_symbols < MAX_SYMBOLS_PER_BLOCK)
    {
        update_hash(h, data[ind + 2]); // Compute new hash
        bool match = mem.head[h] > 0 && (ind - mem.head[h] <= 32768 && data[ind] == data[mem.head[h]] &&
                                         data[ind + 1] == data[mem.head[h] + 1] &&
                                         data[ind + 2] == data[mem.head[h] + 2]);

        // Update hash chain
        mem.prev[ind & window_mask] = mem.head[h];
        mem.head[h] = ind;

        if (!match)
        {
            if (matchOnPreviousByte) // Add the previous and better match
            {
                const int prev_length_code = length_to_length_code(prev_match_len);
                const int prev_dist_code = distance_to_distance_code(prev_match_dist);

                mem.lit_len_frequency_table[prev_length_code]++;
                mem.dist_frequency_table[prev_dist_code]++;
                num_symbols++;
                matches.emplace_back(ind - 1, prev_match_len, prev_match_dist, prev_length_code, prev_dist_code);


                // Add the hashes for the bytes in the match - Hashes for ind - 1 and ind are already added
                for (int i = 0; i < prev_match_len - 2; ++i)
                {
                    ind++;
                    update_hash(h, data[ind + 2]);
                    mem.prev[ind & window_mask] = mem.head[h];
                    mem.head[h] = ind;
                }
            }

            // Add the current byte as a literal
            num_symbols++;
            mem.lit_len_frequency_table[data[ind]]++;
            ind++;
            prev_match_len = 3;
        }
        else
        {
            int curr_match = mem.prev[ind & window_mask];
            int best_match = curr_match;
            int best_len = prev_match_len;
            int chain_length = MAX_CHAIN_LENGTH;

            // Find the best match
            while (curr_match != -1 && curr_match != ind && ind - curr_match <= 32768 && chain_length > 0)
            {
                chain_length--;

                // Check if the match is valid (Discards matches that are too far or hash collisions)
                bool valid = true;
                if (ind + best_len >= size)
                    valid = false;
                if (data[curr_match + best_len] != data[ind + best_len] ||
                    data[curr_match + best_len - 1] != data[ind + best_len - 1] || data[ind] != data[curr_match] ||
                    data[ind + 1] != data[curr_match + 1])
                    valid = false;

                // Advance in the hash chain
                if (!valid)
                {
                    int p = mem.prev[curr_match & window_mask];
                    if (p >= curr_match) // Hash collision
                        break;
                    curr_match = p;
                    continue;
                }

                // Compute match length
                int len = 2;
                while (ind + len < size && data[curr_match + len] == data[ind + len] && len < 258)
                    ++len;

                // Update best match
                if (len > best_len)
                {
                    best_len = len;
                    best_match = curr_match;
                }

                // Advance in the hash chain
                int p = mem.prev[curr_match & window_mask];
                if (p >= curr_match) // Hash collision
                    break;
                curr_match = p;
            }
            const int best_match_dist = ind - best_match;

            if (matchOnPreviousByte)
            {
                if (best_len <= prev_match_len) // Add the previous and better match
                {
                    const int prev_length_code = length_to_length_code(prev_match_len);
                    const int prev_dist_code = distance_to_distance_code(prev_match_dist);

                    mem.lit_len_frequency_table[prev_length_code]++;
                    mem.dist_frequency_table[prev_dist_code]++;
                    num_symbols++;
                    matches.emplace_back(ind - 1, prev_match_len, prev_match_dist, prev_length_code, prev_dist_code);

                    // Add the hashes for the bytes in the match - Hashes for ind - 1 and ind are already added
                    for (int i = 0; i < prev_match_len - 2; ++i)
                    {
                        ind++;
                        update_hash(h, data[ind + 2]);
                        mem.prev[ind & window_mask] = mem.head[h];
                        mem.head[h] = ind;
                    }

                    matchOnPreviousByte = false; // Mark current byte as a literal even though it is a match
                    prev_match_len = 3;
                    ind++;
                    continue;
                }
                else // Add the previous match as a literal
                {
                    mem.lit_len_frequency_table[data[ind - 1]]++;
                    num_symbols++;

                    // Register current match
                    prev_match = best_match;
                    prev_match_len = best_len;
                    prev_match_dist = best_match_dist;

                    ind++;
                }
            }
            else
            {
                // Register current match
                prev_match = best_match;
                prev_match_len = best_len;
                prev_match_dist = best_match_dist;

                ind++;
            }

        }
        matchOnPreviousByte = match;
    }

    if (matchOnPreviousByte) // Add the last match
    {
        const int length_code = length_to_length_code(prev_match_len);
        const int dist_code = distance_to_distance_code(prev_match_dist);

        mem.lit_len_frequency_table[length_code]++;
        mem.dist_frequency_table[dist_code]++;
        num_symbols++;
        matches.emplace_back(ind - 1, prev_match_len, prev_match_dist, length_code, dist_code);
        ind += prev_match_len - 1;
    }

    // Add the last 2 or 1 bytes as literals
    if (ind == size - 2)
    {
        num_symbols++;
        mem.lit_len_frequency_table[data[ind++]]++;
    }
    if (ind == size - 1)
    {
        num_symbols++;
        mem.lit_len_frequency_table[data[ind++]]++;
    }

    if (matches.empty()) // No match -> We provide distances
    {
        // Two distance codes are required to build a tree
        mem.dist_frequency_table[0] = 1;
        mem.dist_frequency_table[1] = 1;
    }

    // Build the Huffman trees
    lit_len_tree = new Huffman_Tree(mem.lit_len_frequency_table, 286);
    dist_tree = new Huffman_Tree(mem.dist_frequency_table, 30);

    return num_symbols < MAX_SYMBOLS_PER_BLOCK ? size - offset : ind - offset;
}

int Deflate::Main::length_to_length_code(const int length)
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

int Deflate::Main::distance_to_distance_code(const int distance)
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

std::string beautiful_size_display(const std::vector<Byte>::size_type size_in_byte)
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

    try
    {
        std::ifstream in;
        for (const auto& file_name : std::filesystem::directory_iterator("../Data/calgaryCorpus"))
        {
            /*if (file_name.path() != "../Data/calgaryCorpus\\pic"*//* && file_name.path() != "../Data/calgaryCorpus\\obj2"*//*)
            continue;*/
            in.open(file_name.path(), std::ios::binary);
            std::cout << std::left << std::setw(20) << file_name.path().filename().string();

            if (!in.is_open())
                throw std::runtime_error("Could not open file!");

            // Read the file
            std::vector<Byte> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            std::cout << std::left << std::setw(15) << beautiful_size_display(data.size());
            in.close();

            // Compress the file block by block
            auto start = std::chrono::steady_clock::now();
            const int data_size = static_cast<int>(data.size());
            std::vector<Byte> compressed = deflate(data.data(), data_size);
            auto end = std::chrono::steady_clock::now();

            // Display results
            std::cout << std::left << std::setw(15) << beautiful_size_display(static_cast<int>(compressed.size()));
            std::cout << std::left << std::setw(15) << std::fixed << std::setprecision(2)
                      <<
                      round((1 - (static_cast<double>(compressed.size()) / static_cast<double>(data.size()))) * 10000) /
                      100;
            std::cout << std::left << std::setw(15)
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

            // Decompress the file to check if it is correct
            std::vector<Byte> decompressed = inflate(compressed);

            for (int i = 0; i < data_size; ++i)
            {
                if (data[i] != decompressed[i])
                    std::cout << "Error at index " << i << "!" << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}

Deflate::Huffman_Tree*
Deflate::Main::code_lengths_to_tree(const int code_lengths[], const int num_symbols, const int max_length)
{
    // Build the code length keys paths
    int* codes = codes_from_code_lengths(code_lengths, num_symbols, max_length);
    auto* code_length_keys_paths = new Deflate::Huffman_Tree::Code[num_symbols];
    for (int i = 0; i < num_symbols; ++i)
    {
        code_length_keys_paths[i] =
                codes[i] == -1 ? Deflate::Huffman_Tree::Code(0, 0) : Deflate::Huffman_Tree::Code(codes[i],
                                                                                                 code_lengths[i]);
    }

    // Build the tree
    auto* tree = new Huffman_Tree(code_length_keys_paths, num_symbols);

    // Clean up
    delete codes;
    delete[] code_length_keys_paths;

    return tree;
}

void
Deflate::Main::read_code_lengths(Deflate::Stream_Reader& reader, const Deflate::Huffman_Tree* tree,
                                 const int num_symbols,
                                 int& max_length, int code_lengths[])
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

int Deflate::Main::enumerate_code_lengths(const int count, const Deflate::Huffman_Tree::Code* codes,
                                          const int max_repetition,
                                          std::vector<std::pair<int, int>>& code_lengths_to_write,
                                          Memory& mem)
{
    int j = 0;
    while (j < count)
    {
        // Count the number of repetitions
        int k = 1;
        while (j + k < count && codes[j + k].length == codes[j].length && k < max_repetition)
            ++k;

        if (k >= 3) // Register a repetition
        {
            // Repetition of 0s
            if (codes[j].length == 0)
            {
                if (j + k == count)
                    break; // Don't add the last 0s
                int code = k < 11 ? 17 : 18;
                mem.code_lengths_frequency_table[code]++;
                mem.dynamic_compression_size += code == 17 ? 3 : 7;
                code_lengths_to_write.emplace_back(code, k);
            }
            else // Other repetitions
            {
                // Code to repeat
                mem.code_lengths_frequency_table[codes[j].length]++;
                mem.dynamic_compression_size += codes[j].length;
                code_lengths_to_write.emplace_back(codes[j].length, 1);

                // Repetition
                const int rep = k - 1;
                const int num_rep = rep / 6;
                mem.code_lengths_frequency_table[16] += num_rep;
                mem.dynamic_compression_size += num_rep * 2;
                for (int i = 0; i < num_rep; ++i)
                    code_lengths_to_write.emplace_back(16, 6);

                // Remaining smaller repetitions
                int l = rep % 6;
                if (l > 2) // Register a repetition of 3, 4 or 5
                {
                    mem.code_lengths_frequency_table[16]++;
                    mem.dynamic_compression_size += 2;
                    code_lengths_to_write.emplace_back(16, l);
                }
                else // Register the last character which is not in any repetition
                {
                    mem.code_lengths_frequency_table[codes[j].length] += l;
                    mem.dynamic_compression_size += l * codes[j].length;
                    for (int i = 0; i < l; ++i)
                        code_lengths_to_write.emplace_back(codes[j].length, 1);
                }
            }
        }
        else // No repetition
        {
            mem.code_lengths_frequency_table[codes[j].length] += k;
            mem.dynamic_compression_size += k * codes[j].length;
            for (int i = 0; i < k; ++i)
                code_lengths_to_write.emplace_back(codes[j].length, 1);
        }

        j += k;
    }


    return j;
}

void Deflate::Main::write_code_lengths(Deflate::Writer& writer, const std::vector<std::pair<int, int>>& code_lengths,
                                       const Deflate::Huffman_Tree::Code* code_length_codes)
{
    for (const auto& [code_length, extra_bits_val] : code_lengths)
    {
        writer.write_code(code_length_codes[code_length].code, code_length_codes[code_length].length);

        switch (code_length)
        {
            case 16: // Repeat last code 3-6 times
                writer.write_number(extra_bits_val - 3, 2);
                break;
            case 17: // Repeat 0 3-10 times
                writer.write_number(extra_bits_val - 3, 3);
                break;
            case 18: // Repeat 0 11-138 times
                writer.write_number(extra_bits_val - 11, 7);
                break;
            default:
                break;
        }
    }
}

void Deflate::Main::write_compressed_data(Deflate::Writer& writer, const Byte* data, const int offset,
                                          const int size,
                                          const std::list<Match>& matches,
                                          const Deflate::Huffman_Tree::Code* lit_len_codes,
                                          const Deflate::Huffman_Tree::Code* distance_codes)
{
    int index = offset;
    int nextMatchPos = matches.empty() ? size : matches.front().position();
    auto nextMatchIter = matches.begin();
    while (index < offset + size)
    {
        if (index == nextMatchPos) // Match
        {
            // Write the match
            const int length = (*nextMatchIter).length();
            const int length_code_value = nextMatchIter->length_code();
            writer.write_code(lit_len_codes[length_code_value].code, lit_len_codes[length_code_value].length);

            // Write the extra bits
            const int extra_bits_value = length - lit_len_code_to_length[length_code_value];
            const int num_extra_bits = lengths_extra_bits[length_code_value];
            writer.write_number(extra_bits_value, num_extra_bits);

            // Write the distance
            const int distance = (*nextMatchIter).distance();
            const int distance_code_value = distance_to_distance_code(distance);
            writer.write_code(distance_codes[distance_code_value].code, distance_codes[distance_code_value].length);

            // Write the extra bits
            const int distance_extra_bits_value = distance - dist_code_to_dist[distance_code_value];
            const int num_distance_extra_bits = distances_extra_bits[distance_code_value];
            writer.write_number(distance_extra_bits_value, num_distance_extra_bits);

            index += (*nextMatchIter).length();
            nextMatchPos = ++nextMatchIter == matches.end() ? size : (*nextMatchIter).position();
        }
        else // Literal
        {
            writer.write_code(lit_len_codes[data[index]].code, lit_len_codes[data[index]].length);
            index++;
        }
    }
}

void Deflate::Main::Test_file(const std::string& file_name, const bool verify_compression = false)
{
    std::ifstream file(file_name, std::ios::in | std::ios::binary | std::ios::ate);
    std::streampos size = file.tellg();
    Byte* data = nullptr;

    if (file.is_open())
    {
        data = new Byte[size];
        file.seekg(0, std::ios::beg);
        file.read((char*) data, size);
        file.close();
    }
    else
        std::cout << "Unable to open file" << std::endl;

    // Compression
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    auto compressed = Deflate::Main::deflate(data, static_cast<int>(size));
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    // Show results
    std::cout << "Duration = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << "[ms]" << std::endl;
    std::cout << "Compressed size: " << compressed.size() << " File size: " << size << std::endl;
    std::cout << "Compression ratio: " << std::fixed << std::setprecision(2)
              << round((1 - (static_cast<double>(compressed.size()) / static_cast<double>(size))) * 10000) /
                 100 << "%" << std::endl;

    // Decompression
    if (verify_compression)
    {
        std::cout << std::endl << "Verifying compression..." << std::endl;

        auto decompressed = Deflate::Main::inflate(compressed);

        bool no_error = false;
        if (decompressed.size() != size)
            std::cout << "Decompressed size is not equal to original size" << std::endl;
        else
        {
            int i;
            for (i = 0; i < size; ++i)
            {
                if (decompressed[i] != data[i])
                {
                    std::cout << "Decompressed data is not equal to original data" << std::endl;
                    break;
                }
            }
            if (i == size)
                no_error = true;
        }

        if (no_error)
            std::cout << "No error found" << std::endl;
    }

    // Free memory
    delete[] data;
}

Deflate::Main::dynamic_comp_res
Deflate::Main::compute_dynamic_comp_data(const Byte* data, int data_size, Writer& writer, const int offset,
                                         Memory& mem)
{
    int ind = offset;

    // Compute the dynamic trees by finding the matches
    Huffman_Tree* lit_len_tree;
    Huffman_Tree* dist_tree;
    auto* matches = new std::list<Match>();
    int uncompressed_block_size = compute_dynamic_trees(data, ind, data_size, *matches, lit_len_tree, dist_tree,
                                                        mem);

    // Compute canonical codes
    Deflate::Huffman_Tree::Code* lit_len_codes = lit_len_tree->canonical_codes(286, MAX_CODE_LENGTH);
    Deflate::Huffman_Tree::Code* distance_codes = dist_tree->canonical_codes(30, MAX_CODE_LENGTH);


    // Compute lit/len + beginning of code length frequency table
    auto* lit_len_code_lengths_to_write = new std::vector<std::pair<int, int>>();
    const int provided_lit_len = enumerate_code_lengths(286, lit_len_codes, 138, *lit_len_code_lengths_to_write, mem);
    //Todo: Change enum signature to take mem in order to do part of the computation of dynamic_compression_size
    // Make the whole computation of static and dynamic compression size in this function

    // Compute distance + end of code length frequency table
    auto* dist_code_lengths_to_write = new std::vector<std::pair<int, int>>();
    const int provided_dist_codes = enumerate_code_lengths(30, distance_codes, 30,
                                                           *dist_code_lengths_to_write, mem);

    // Code length code lengths
    Huffman_Tree code_lengths_tree(mem.code_lengths_frequency_table, 19);
    Deflate::Huffman_Tree::Code* code_length_codes = code_lengths_tree.canonical_codes(19, MAX_CODE_LENGTH_CODE_LENGTH);


    // Determine the number of code length code lengths to write
    int num_code_length_code_length_to_write = 19;
    while (code_length_codes[code_length_codes_order[num_code_length_code_length_to_write - 1]].length == 0 &&
           num_code_length_code_length_to_write >= 0)
        --num_code_length_code_length_to_write;
    mem.dynamic_compression_size += 3 * num_code_length_code_length_to_write;


    // Save the computed information
    dynamic_comp_res res = dynamic_comp_res(offset, uncompressed_block_size, provided_lit_len, provided_dist_codes,
                                            num_code_length_code_length_to_write, lit_len_code_lengths_to_write,
                                            dist_code_lengths_to_write, code_length_codes, lit_len_codes,
                                            distance_codes, matches, offset + uncompressed_block_size == data_size);
    // Free memory
    delete lit_len_tree;
    delete dist_tree;

    return res;
}

void
Deflate::Main::deflate_fixed(const Byte* data, Deflate::Writer& writer, const dynamic_comp_res& dynamic_comp_res,
                             const int compressed_size)
{
    // Avoid reallocation
    writer.data->reserve(writer.data->size() + compressed_size);

    writer.write_number(dynamic_comp_res.is_last_block, 1); // BFINAL
    writer.write_number(1, 2); // BTYPE

    int ind = dynamic_comp_res.offset;
    int nextMatchPos = dynamic_comp_res.matches->empty() ? dynamic_comp_res.uncompressed_size
                                                         : dynamic_comp_res.matches->front().position();
    auto nextMatchIter = dynamic_comp_res.matches->begin();
    const int block_end = dynamic_comp_res.offset + dynamic_comp_res.uncompressed_size;

    // Write the block
    while (ind < block_end)
    {
        if (ind == nextMatchPos) // Match
        {
            const int length = nextMatchIter->length();
            const int distance = nextMatchIter->distance();
            const int length_code = nextMatchIter->length_code();
            const int dist_code = nextMatchIter->dist_code();

            // Length
            writer.write_code(fixed_lit_len_values_codes[length_code],
                              lit_len_fixed_code_length(length_code)); // Length code
            writer.write_number(length - lit_len_code_to_length[length_code],
                                lengths_extra_bits[length_code]); // Length extra
            // Distance
            writer.write_number(dist_code, 5); // Distance code
            writer.write_number(distance - dist_code_to_dist[dist_code],
                                distances_extra_bits[dist_code]); // Distance extra

            ind += length;
            nextMatchIter++;
            nextMatchPos = nextMatchIter == dynamic_comp_res.matches->end() ? dynamic_comp_res.uncompressed_size
                                                                            : nextMatchIter->position();
        }
        else // Literal
        {
            writer.write_code(fixed_lit_len_values_codes[data[ind]], lit_len_fixed_code_length(data[ind]));
            ind++;
        }
    }

    writer.write_code(fixed_lit_len_values_codes[256], lit_len_fixed_code_length(256)); // End of block
}

void
Deflate::Main::deflate_uncompressed(const Byte* data, const int num_bytes, const int offset, const bool is_last_block,
                                    Writer& writer)
{
    // Avoid reallocation
    writer.data->reserve(writer.data->size() + num_bytes);

    writer.write_number(is_last_block, 1); // BFINAL
    writer.write_number(0, 2); // BTYPE

    writer.write_curr_byte_if_not_empty(); // Padding

    writer.write_number(num_bytes, 16); // LEN
    writer.write_number(~num_bytes, 16); // NLEN

    // Write the block
    for (int i = 0; i < num_bytes; ++i)
        writer.write_raw_byte(data[offset + i]);
}

int Deflate::Main::compressed_size_with_fixed_codes(const Byte* data, const dynamic_comp_res& dynamic_comp_res)
{
    int ind = dynamic_comp_res.offset;
    int compressed_size = 3; // BFINAL & BTYPE

    int nextMatchPos = dynamic_comp_res.matches->empty() ? dynamic_comp_res.uncompressed_size
                                                         : dynamic_comp_res.matches->front().position();
    auto nextMatchIter = dynamic_comp_res.matches->begin();


    // Compute the compressed size
    const int block_end = dynamic_comp_res.offset + dynamic_comp_res.uncompressed_size;
    while (ind < block_end)
    {
        if (ind == nextMatchPos) // Match
        {
            const int length = nextMatchIter->length();
            const int distance = nextMatchIter->distance();
            const int length_code = nextMatchIter->length_code();
            const int distance_code = nextMatchIter->dist_code();

            // Length
            compressed_size += lit_len_fixed_code_length(length_code); // Length code length
            compressed_size += lengths_extra_bits[length_code]; // Length extra bits
            // Distance
            compressed_size += 5; // Distance code length
            compressed_size += distances_extra_bits[distance_code]; // Distance extra bits
            ind += (*nextMatchIter).length();
            nextMatchPos = ++nextMatchIter == dynamic_comp_res.matches->end() ? dynamic_comp_res.uncompressed_size
                                                                              : (*nextMatchIter).position();
        }
        else
        {
            compressed_size += lit_len_fixed_code_length(data[ind]);
            ++ind;
        }
    }

    compressed_size += lit_len_fixed_code_length(256); // End of block

    return compressed_size / 8; // In bytes
}

int Deflate::Main::lit_len_fixed_code_length(const int lit_len)
{
    if (lit_len <= 143 || lit_len >= 280) // 0-143 & 280-287
        return 8;
    else if (lit_len <= 255) // 144-255
        return 9;
    else if (lit_len <= 279) // 256-279
        return 7;

    throw std::runtime_error("Invalid literal/length");
}

void Deflate::Main::deflate_dynamic(const dynamic_comp_res& dynamic_comp_res, Deflate::Writer& writer, const Byte* data,
                                    const int compressed_size)
{
    // Avoid reallocation
    writer.data->reserve(writer.data->size() + compressed_size);

    writer.write_number(dynamic_comp_res.is_last_block, 1); // BFINAL
    writer.write_number(2, 2); // BTYPE
    writer.write_number(dynamic_comp_res.provided_lit_len - 257, 5); // HLIT
    writer.write_number(dynamic_comp_res.provided_dist_codes - 1, 5); // HDIST
    writer.write_number(dynamic_comp_res.num_code_length_code_length_to_write - 4, 4); // HCLEN

    // Write the code length code lengths
    for (int k = 0; k < dynamic_comp_res.num_code_length_code_length_to_write; ++k)
        writer.write_number(dynamic_comp_res.code_length_codes[code_length_codes_order[k]].length, 3);

    // Write lit/len code lengths
    write_code_lengths(writer, *dynamic_comp_res.lit_len_code_lengths_to_write,
                       dynamic_comp_res.code_length_codes);
    // Write distance code lengths
    write_code_lengths(writer, *dynamic_comp_res.dist_code_lengths_to_write,
                       dynamic_comp_res.code_length_codes);

    // Write the compressed data
    write_compressed_data(writer, data, dynamic_comp_res.offset, dynamic_comp_res.uncompressed_size,
                          *dynamic_comp_res.matches,
                          dynamic_comp_res.lit_len_codes,
                          dynamic_comp_res.dist_codes);

    // Write end of block
    writer.write_code(dynamic_comp_res.lit_len_codes[256].code,
                      dynamic_comp_res.lit_len_codes[256].length);
}

int Deflate::Main::compressed_size_with_dynamic_codes(const Byte* data,
                                                      const Deflate::Main::dynamic_comp_res& dynamic_comp_res)
{
    int ind = dynamic_comp_res.offset;
    int compressed_size = 0;

    compressed_size += 3; // BFINAL & BTYPE
    compressed_size += 10; // HLIT & HDIST
    compressed_size += 4; // HCLEN
    compressed_size += dynamic_comp_res.num_code_length_code_length_to_write * 3; // Code length code lengths

    // Lit/len code lengths
    for (const auto& [code_length, extra_bits_val] : (*dynamic_comp_res.lit_len_code_lengths_to_write))
    {
        compressed_size += dynamic_comp_res.code_length_codes[code_length].length;
        switch (code_length)
        {
            case 16:
                compressed_size += 2;
                break;
            case 17:
                compressed_size += 3;
                break;
            case 18:
                compressed_size += 7;
                break;
            default:
                break;
        }
    }
    // Dist code lengths
    for (const auto& [code_length, extra_bits_val] : (*dynamic_comp_res.dist_code_lengths_to_write))
    {
        compressed_size += dynamic_comp_res.code_length_codes[code_length].length;
        switch (code_length)
        {
            case 16:
                compressed_size += 2;
                break;
            case 17:
                compressed_size += 3;
                break;
            case 18:
                compressed_size += 7;
                break;
            default:
                break;
        }
    }

    auto nextMatchIter = dynamic_comp_res.matches->begin();
    int nextMatchPos = dynamic_comp_res.matches->empty() ? dynamic_comp_res.uncompressed_size
                                                         : dynamic_comp_res.matches->front().position();

    // Compute the compressed size
    const int block_end = dynamic_comp_res.offset + dynamic_comp_res.uncompressed_size;
    while (ind < block_end)
    {
        if (ind == nextMatchPos) // Match
        {
            const int length_code = nextMatchIter->length_code();
            const int dist_code = nextMatchIter->dist_code();

            // Add the size of the match
            compressed_size += dynamic_comp_res.lit_len_codes[length_code].length;
            compressed_size += dynamic_comp_res.dist_codes[dist_code].length;
            compressed_size += lengths_extra_bits[length_code];
            compressed_size += distances_extra_bits[dist_code];

            ind += (*nextMatchIter).length();
            nextMatchPos = ++nextMatchIter == dynamic_comp_res.matches->end() ? dynamic_comp_res.uncompressed_size
                                                                              : (*nextMatchIter).position();
        }
        else
        {
            compressed_size += dynamic_comp_res.lit_len_codes[data[ind]].length;
            ++ind;
        }
    }

    compressed_size += dynamic_comp_res.lit_len_codes[256].length; // End of block

    return compressed_size / 8; // In bytes
}

void Deflate::Main::build_fixed_huffman_lit_len_values_codes()
{
    int c = 0;
    for (int i = 0b00110000; i <= 0b10111111; i++)
        fixed_lit_len_values_codes[c++] = i;
    for (int i = 0b110010000; i <= 0b111111111; i++)
        fixed_lit_len_values_codes[c++] = i;
    for (int i = 0b0000000; i <= 0b0010111; i++)
        fixed_lit_len_values_codes[c++] = i;
    for (int i = 0b11000000; i <= 0b11000111; i++)
        fixed_lit_len_values_codes[c++] = i;
}


Deflate::Main::dynamic_comp_res::dynamic_comp_res(const int offset, const int uncompressedSize,
                                                  const int providedLitLen, const int providedDistCodes,
                                                  const int numCodeLengthCodeLengthToWrite,
                                                  const std::vector<std::pair<int, int>>* litLenCodeLengthsToWrite,
                                                  const std::vector<std::pair<int, int>>* distCodeLengthsToWrite,
                                                  const Deflate::Huffman_Tree::Code* codeLengthCodes,
                                                  const Deflate::Huffman_Tree::Code* litLenCodes,
                                                  const Deflate::Huffman_Tree::Code* distCodes,
                                                  const std::list<Deflate::Match>* matches, const bool is_last_block)
        : offset(offset),
          uncompressed_size(
                  uncompressedSize),
          provided_lit_len(
                  providedLitLen),
          provided_dist_codes(
                  providedDistCodes),
          num_code_length_code_length_to_write(
                  numCodeLengthCodeLengthToWrite),
          lit_len_code_lengths_to_write(
                  litLenCodeLengthsToWrite),
          dist_code_lengths_to_write(
                  distCodeLengthsToWrite),
          code_length_codes(
                  codeLengthCodes),
          lit_len_codes(
                  litLenCodes),
          dist_codes(distCodes),
          matches(matches),
          is_last_block(is_last_block)
{}

Deflate::Main::dynamic_comp_res::~dynamic_comp_res()
{
    delete[] code_length_codes;
    delete[] lit_len_codes;
    delete[] dist_codes;
    delete matches;
    delete lit_len_code_lengths_to_write;
    delete dist_code_lengths_to_write;
}
