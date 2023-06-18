#include "Main.h"
#include "HuffmanTree.h"
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
Deflate::Main::decompressBlock(StreamReader& reader, Window& window, Writer& writer)
{
    const bool is_final_block = reader.readBit();

    switch (reader.readNumber(2))
    {
        case 0:
            getStoredData(reader, writer);
            break;
        case 1:
            getFixedHuffmanData(reader, window, writer);
            break;
        case 2:
            getDynamicHuffmanData(reader, window, writer);
            break;
        case 3:
            throw std::runtime_error("Reserved block type!");
        default: // Should never happen
            throw std::runtime_error("Unknown block type!");
    }

    return is_final_block;
}

void Deflate::Main::getStoredData(StreamReader& reader, Writer& writer)
{
    reader.skipEndOfByte(); // Padding

    // Read data length
    const auto len = static_cast<short>(reader.readNumber(16));
    const auto nlen = static_cast<short>(reader.readNumber(16));
    if (len != ~nlen)
        throw std::runtime_error("Invalid stored block length!");

    // Read data
    for (const Byte& b : reader.readBytesV(len))
        writer.write_raw_byte(b);
}

void Deflate::Main::getFixedHuffmanData(StreamReader& reader, Window& window, Writer& writer)
{
    int lit_len = static_huffman_tree_->readKey(reader);
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
                len += reader.readNumber(extra_bits);

            // Distance
            const int dist_value = reader.readNumber(5);
            /*if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");*/
            int dist = dist_code_to_dist[dist_value];
            if (const int extra_bits = distances_extra_bits[dist_value]) // Extra bits
                dist += reader.readNumber(extra_bits);

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

        lit_len = static_huffman_tree_->readKey(reader);
    }
}

void Deflate::Main::getDynamicHuffmanData(StreamReader& reader, Window& window, Writer& writer)
{
    // Read the number of different codes to read
    const int num_lit_len = reader.readNumber(5) + 257;
    const int num_dist = reader.readNumber(5) + 1;
    const int num_code_length_code_length = reader.readNumber(4) + 4;

    // Read code length code lengths
    int max_code_length_code_length = 0;
    int code_length_code_lengths[19];
    for (int i = 0; i < num_code_length_code_length; ++i)
    {
        int code_length_code_length = reader.readNumber(3);
        code_length_code_lengths[code_length_codes_order[i]] = code_length_code_length;
        if (code_length_code_length > max_code_length_code_length)
            max_code_length_code_length = code_length_code_length;
    }
    for (int i = num_code_length_code_length; i < 19; ++i)
        code_length_code_lengths[code_length_codes_order[i]] = 0;

    // Build code length tree with the lengths of the code length codes
    HuffmanTree* code_lengths_tree = codeLengthsToTree(code_length_code_lengths, 19, max_code_length_code_length);

    // Read lit_len code lengths
    int lit_len_code_lengths[num_lit_len];
    int max_lit_len_code_length = 0;
    readCodeLengths(reader, code_lengths_tree, num_lit_len, max_lit_len_code_length, lit_len_code_lengths);

    // Build lit_len tree with the code lengths
    HuffmanTree* lit_len_tree = codeLengthsToTree(lit_len_code_lengths, num_lit_len, max_lit_len_code_length);

    // Read dist code lengths
    int dist_code_lengths[num_dist];
    int max_dist_code_length = 0;
    readCodeLengths(reader, code_lengths_tree, num_dist, max_dist_code_length, dist_code_lengths);

    // Build dist tree with the code lengths
    HuffmanTree* dist_tree = codeLengthsToTree(dist_code_lengths, num_dist, max_dist_code_length);

    // Read the data
    readDynamicHuffmanData(reader, lit_len_tree, dist_tree, window, writer);

    // Free memory
    delete lit_len_tree;
    delete dist_tree;
    delete code_lengths_tree;
}

void Deflate::Main::buildFixedHuffmanTree()
{
    auto* keys_paths = new Deflate::HuffmanTree::Code[288];
    int c = 0;
    for (int i = 0b00110000; i <= 0b10111111; i++)
        keys_paths[c++] = {i, 8};
    for (int i = 0b110010000; i <= 0b111111111; i++)
        keys_paths[c++] = {i, 9};
    for (int i = 0b0000000; i <= 0b0010111; i++)
        keys_paths[c++] = {i, 7};
    for (int i = 0b11000000; i <= 0b11000111; i++)
        keys_paths[c++] = {i, 8};

    static_huffman_tree_ = new HuffmanTree(keys_paths, 288);
}

std::vector<Byte> Deflate::Main::deflate(const Byte* data, int size)
{
    // Build fixed huffman lit len values codes if not done yet
    if (fixed_lit_len_values_codes.empty())
        buildFixedHuffmanLitLenValuesCodes();

    int ind = 0;
    std::vector<Byte> compressed_data;
    Writer writer(&compressed_data);
    Memory mem{};
    // While there is still data to compress
    while (ind < size)
    {
        // Compute dynamic compression data
        const CompressionInfo compression_info = processBlock(data, size, writer, ind, mem);
        // Compute compressed size with dynamic and fixed codes

        // Select best compression mode
        // Dynamic codes are only used if they are smaller than fixed codes and the uncompressed data
        if (compression_info.dynamic_compression_size >= compression_info.fixed_compression_size ||
            compression_info.dynamic_compression_size >= compression_info.uncompressed_size)
        {
            // Use fixed codes if they are smaller than the uncompressed data
            if (compression_info.fixed_compression_size >= compression_info.uncompressed_size)
                deflateUncompressed(data, ind, writer, compression_info);

            else // No compression if both static and dynamic do not reduce the size
                deflateFixed(data, writer, compression_info, mem);
        }

        else
            deflateDynamic(compression_info, writer, data, mem);

        ind += compression_info.uncompressed_size;
        mem.clean();
    }

    writer.close(); // Close the writer to write the last byte
    return compressed_data;
}

std::vector<Byte> Deflate::Main::inflate(std::vector<Byte> data)
{
    // Build static huffman data if not done yet
    if (static_huffman_tree_ == nullptr)
        buildFixedHuffmanTree();
    try
    {
        std::vector<Byte> inflated_data;
        Writer writer(&inflated_data);
        StreamReader reader(&data);
        Window window{};
        do
        {
        } while (!decompressBlock(reader, window, writer)); // Decompress blocks until the last block is reached

        return inflated_data;
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
        throw;
    }
}

int* Deflate::Main::codesFromCodeLengths(const int code_lengths[], const int num_symbols,
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
Deflate::Main::readDynamicHuffmanData(StreamReader& reader, const HuffmanTree* lit_len_tree,
                                      const HuffmanTree* dist_tree,
                                      Window& window,
                                      Writer& writer)
{
    int lit_len = lit_len_tree->readKey(reader);
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
                len += reader.readNumber(extra_bits);

            // Distance
            const int dist_value = dist_tree->readKey(reader);
            /*if (dist_value > 29)
                throw std::runtime_error("Invalid distance!");*/
            int dist = dist_code_to_dist[dist_value];
            if (const int extra_bits = distances_extra_bits[dist_value])
                dist += reader.readNumber(extra_bits);

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

        lit_len = lit_len_tree->readKey(reader);
    }
}

int
Deflate::Main::computeDynamicTrees(const Byte* data, const int offset, const int size, HuffmanTree*& lit_len_tree,
                                   HuffmanTree*& dist_tree, Memory& mem)
{
    bool matchOnPreviousByte = false;
    int ind = offset;
    mem.lit_len_frequency_table[256] = 1; // End of block
    int h = data[ind];

    int prev_match = -1;
    int prev_match_len = 2;
    int prev_match_dist = 0;

    updateHash(h, data[ind + 1]); // Update hash of first two bytes

    // Find matches
    while (ind < size - 2 && mem.num_symbols < MAX_SYMBOLS_PER_BLOCK)
    {
        updateHash(h, data[ind + 2]); // Compute new hash
        bool match = mem.head[h] > 0 && (ind - mem.head[h] <= 32768 && data[ind] == data[mem.head[h]] &&
                                         data[ind + 1] == data[mem.head[h] + 1] &&
                                         data[ind + 2] == data[mem.head[h] + 2]);

        // Update hash chain
        mem.prev[ind & window_mask] = mem.head[h];
        mem.head[h] = ind;

        int best_len = matchOnPreviousByte ? prev_match_len : 2;

        if (match)
        {
            int best_match_dist = 0;
            int best_match = mem.prev[ind & window_mask];

            findBestMatch(data, mem, best_len, best_match, best_match_dist, ind, size);

            if (matchOnPreviousByte)
            {
                if (best_len <= prev_match_len) // Add the previous and better match
                {
                    savePreviousMatch(prev_match, prev_match_len, prev_match_dist, data, ind, mem, h);
                    best_len = 2;
                }
                else // Add the previous match as a literal
                {
                    mem.lit_len_frequency_table[data[ind - 1]]++;
                    mem.symbols[mem.num_symbols++] = Match(data[ind - 1]);

                    // Register current match
                    prev_match = best_match;
                    prev_match_len = best_len;
                    prev_match_dist = best_match_dist;
                }
            }
            else
            {
                // Register current match
                prev_match = best_match;
                prev_match_len = best_len;
                prev_match_dist = best_match_dist;
            }

        }
        else
        {
            if (matchOnPreviousByte) // Add the previous match
                savePreviousMatch(prev_match, prev_match_len, prev_match_dist, data, ind, mem, h);
            else
            {
                // Add the current byte as a literal
                mem.symbols[mem.num_symbols++] = Match(data[ind]);
                mem.lit_len_frequency_table[data[ind]]++;
            }
        }

        ind++;
        matchOnPreviousByte = match && best_len >= 3;
    }

    if (matchOnPreviousByte) // Add the last match
    {
        if (mem.num_symbols < MAX_SYMBOLS_PER_BLOCK) // Add the last match
        {
            const int length_code = lengthToLengthCode(prev_match_len);
            const int dist_code = distanceToDistanceCode(prev_match_dist);

            mem.lit_len_frequency_table[length_code]++;
            mem.dist_frequency_table[dist_code]++;
            mem.symbols[mem.num_symbols++] = Match(data[ind - 1], prev_match_len, prev_match_dist, length_code,
                                                   dist_code);
            ind += prev_match_len - 1;
        }
        else ind--; // As the last match has not been added, we go back one byte
    }

    // Add the last 2 or 1 bytes as literals
    if (ind == size - 2 && mem.num_symbols < MAX_SYMBOLS_PER_BLOCK)
    {
        mem.symbols[mem.num_symbols++] = Match(data[ind]);
        mem.lit_len_frequency_table[data[ind++]]++;
    }
    if (ind == size - 1 && mem.num_symbols < MAX_SYMBOLS_PER_BLOCK)
    {
        mem.symbols[mem.num_symbols++] = Match(data[ind]);
        mem.lit_len_frequency_table[data[ind++]]++;
    }

    bool at_least_one_match;
    for (int i : mem.dist_frequency_table)
    {
        if (i > 0)
        {
            at_least_one_match = true;
            break;
        }
    }
    if (!at_least_one_match) // No match -> We provide distances
    {
        // Two distance codes are required to build a tree
        mem.dist_frequency_table[0] = 1;
        mem.dist_frequency_table[1] = 1;
    }

    // Build the Huffman trees
    lit_len_tree = new HuffmanTree(mem.lit_len_frequency_table, 286);
    dist_tree = new HuffmanTree(mem.dist_frequency_table, 30);

    return ind - offset;
}

int Deflate::Main::lengthToLengthCode(const int length)
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

int Deflate::Main::distanceToDistanceCode(const int distance)
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

void Deflate::Main::test()
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

Deflate::HuffmanTree*
Deflate::Main::codeLengthsToTree(const int code_lengths[], const int num_symbols, const int max_length)
{
    // Build the code length keys paths
    int* codes = codesFromCodeLengths(code_lengths, num_symbols, max_length);
    auto* code_length_keys_paths = new Deflate::HuffmanTree::Code[num_symbols];
    for (int i = 0; i < num_symbols; ++i)
    {
        code_length_keys_paths[i] =
                codes[i] == -1 ? Deflate::HuffmanTree::Code(0, 0) : Deflate::HuffmanTree::Code(codes[i],
                                                                                               code_lengths[i]);
    }

    // Build the tree
    auto* tree = new HuffmanTree(code_length_keys_paths, num_symbols);

    // clean up
    delete codes;
    delete[] code_length_keys_paths;

    return tree;
}

void
Deflate::Main::readCodeLengths(Deflate::StreamReader& reader, const Deflate::HuffmanTree* tree,
                               int num_symbols,
                               int& max_length, int code_lengths[])
{
    int read_codes = 0;
    while (read_codes < num_symbols)
    {
        const int code = tree->readKey(reader);
        switch (code)
        {
            case 16: // Repeat last code 3-6 times
            {
                const int repeat = reader.readNumber(2) + 3;
                for (int i = 0; i < repeat; ++i)
                    code_lengths[read_codes++] = code_lengths[read_codes - 1];
                break;
            }
            case 17: // Repeat 0 3-10 times
            {
                const int repeat = reader.readNumber(3) + 3;
                for (int i = 0; i < repeat; ++i)
                    code_lengths[read_codes++] = 0;
                break;
            }
            case 18: // Repeat 0 11-138 times
            {
                const int repeat = reader.readNumber(7) + 11;
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

int Deflate::Main::enumerateCodeLengths(const int count, const Deflate::HuffmanTree::Code* codes,
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
                code_lengths_to_write.emplace_back(code, k);
            }
            else // Other repetitions
            {
                // Code to repeat
                mem.code_lengths_frequency_table[codes[j].length]++;
                code_lengths_to_write.emplace_back(codes[j].length, 1);

                // Repetition
                const int rep = k - 1;
                const int num_rep = rep / 6;
                mem.code_lengths_frequency_table[16] += num_rep;
                for (int i = 0; i < num_rep; ++i)
                    code_lengths_to_write.emplace_back(16, 6);

                // Remaining smaller repetitions
                int l = rep % 6;
                if (l > 2) // Register a repetition of 3, 4 or 5
                {
                    mem.code_lengths_frequency_table[16]++;
                    code_lengths_to_write.emplace_back(16, l);
                }
                else // Register the last character which is not in any repetition
                {
                    mem.code_lengths_frequency_table[codes[j].length] += l;
                    for (int i = 0; i < l; ++i)
                        code_lengths_to_write.emplace_back(codes[j].length, 1);
                }
            }
        }
        else // No repetition
        {
            mem.code_lengths_frequency_table[codes[j].length] += k;
            for (int i = 0; i < k; ++i)
                code_lengths_to_write.emplace_back(codes[j].length, 1);
        }

        j += k;
    }


    return j;
}

void Deflate::Main::writeCodeLengths(Deflate::Writer& writer, const std::vector<std::pair<int, int>>& code_lengths,
                                     const Deflate::HuffmanTree::Code* code_length_codes)
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

void Deflate::Main::writeCompressedData(Deflate::Writer& writer, const Byte* data, const int offset, const int size,
                                        const Deflate::HuffmanTree::Code* lit_len_codes,
                                        const Deflate::HuffmanTree::Code* distance_codes, const Memory& mem)
{
    for (int i = 0; i < mem.num_symbols; ++i)
    {
        const Match* m = &mem.symbols[i];

        if (m->length() == 0)
            writer.write_code(lit_len_codes[m->val()].code, lit_len_codes[m->val()].length);
        else
        {
            // Write the match
            const int length = m->length();
            const int length_code_value = m->length_code();
            writer.write_code(lit_len_codes[length_code_value].code, lit_len_codes[length_code_value].length);

            // Write the extra bits
            const int extra_bits_value = length - lit_len_code_to_length[length_code_value];
            const int num_extra_bits = lengths_extra_bits[length_code_value];
            writer.write_number(extra_bits_value, num_extra_bits);

            // Write the distance
            const int distance = m->distance();
            const int distance_code_value = m->dist_code();
            writer.write_code(distance_codes[distance_code_value].code, distance_codes[distance_code_value].length);

            // Write the extra bits
            const int distance_extra_bits_value = distance - dist_code_to_dist[distance_code_value];
            const int num_distance_extra_bits = distances_extra_bits[distance_code_value];
            writer.write_number(distance_extra_bits_value, num_distance_extra_bits);
        }
    }
}

void Deflate::Main::testFile(const std::string& file_name, bool verify_compression = false)
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

Deflate::Main::CompressionInfo
Deflate::Main::processBlock(const Byte* data, int data_size, Writer& writer, const int offset,
                            Memory& mem)
{
    int ind = offset;
    int dynamic_compression_size = 1 + 2 + 10 + 4; // BFINAL BTYPE HLIT HDIST HCLEN
    int fixed_compression_size = 3; // BFINAL BTYPE

    // Compute the dynamic trees by finding the matches
    HuffmanTree* lit_len_tree;
    HuffmanTree* dist_tree;
    int uncompressed_block_size = computeDynamicTrees(data, ind, data_size, lit_len_tree, dist_tree,
                                                      mem);

    // Compute canonical codes
    Deflate::HuffmanTree::Code* lit_len_codes = lit_len_tree->canonicalCodes(286, MAX_CODE_LENGTH);
    Deflate::HuffmanTree::Code* distance_codes = dist_tree->canonicalCodes(30, MAX_CODE_LENGTH);


    // Compute lit/len + beginning of code length frequency table
    auto* lit_len_code_lengths_to_write = new std::vector<std::pair<int, int>>();
    const int provided_lit_len = enumerateCodeLengths(286, lit_len_codes, 138, *lit_len_code_lengths_to_write, mem);

    // Compute distance + end of code length frequency table
    auto* dist_code_lengths_to_write = new std::vector<std::pair<int, int>>();
    const int provided_dist_codes = enumerateCodeLengths(30, distance_codes, 30,
                                                         *dist_code_lengths_to_write, mem);

    // Code length code lengths
    HuffmanTree code_lengths_tree(mem.code_lengths_frequency_table, 19);
    Deflate::HuffmanTree::Code* code_length_codes = code_lengths_tree.canonicalCodes(19, MAX_CODE_LENGTH_CODE_LENGTH);


    // Determine the number of code length code lengths to write
    int num_code_length_code_length_to_write = 19;
    while (code_length_codes[code_length_codes_order[num_code_length_code_length_to_write - 1]].length == 0 &&
           num_code_length_code_length_to_write >= 0)
        --num_code_length_code_length_to_write;

    // Compute the dynamic compression size of the code length code lengths
    dynamic_compression_size += 3 * num_code_length_code_length_to_write;

    // Compute the dynamic compression size of the code lengths
    for (const auto& code_lengths_to_write : {lit_len_code_lengths_to_write, dist_code_lengths_to_write})
    {
        for (const auto& [code_length, extra_bits_val] : *code_lengths_to_write)
        {
            dynamic_compression_size += code_length_codes[code_length].length;
            switch (code_length)
            {
                case 16:
                    dynamic_compression_size += 2;
                    break;
                case 17:
                    dynamic_compression_size += 3;
                    break;
                case 18:
                    dynamic_compression_size += 7;
                    break;
                default:
                    break;
            }
        }
    }

    // Compute the dynamic and fixed compression size of the data itself
    for (int i = 0; i < mem.num_symbols; ++i)
    {
        const Match* m = &mem.symbols[i];

        if (m->length() == 0)
        {
            // Add the size of the literal
            dynamic_compression_size += lit_len_codes[m->val()].length;
            fixed_compression_size += litLenFixedCodeLength(m->val());
        }
        else
        {
            // Add the size of the match
            const int length_code = m->length_code();
            const int dist_code = m->dist_code();

            // Dynamic
            dynamic_compression_size += lit_len_codes[length_code].length;
            dynamic_compression_size += distance_codes[dist_code].length;
            dynamic_compression_size += lengths_extra_bits[length_code];
            dynamic_compression_size += distances_extra_bits[dist_code];

            // Fixed
            fixed_compression_size += litLenFixedCodeLength(length_code); // Length code length
            fixed_compression_size += lengths_extra_bits[length_code]; // Length extra bits
            fixed_compression_size += 5; // Distance code length
            fixed_compression_size += distances_extra_bits[dist_code]; // Distance extra bits
        }
    }

    // Add the size of the end of block
    dynamic_compression_size += lit_len_codes[256].length;
    fixed_compression_size += litLenFixedCodeLength(256);

    // Divide by 8 to get the number of bytes
    dynamic_compression_size /= 8;
    fixed_compression_size /= 8;


    // Save the computed information
    CompressionInfo res = CompressionInfo(offset, uncompressed_block_size, provided_lit_len, provided_dist_codes,
                                          num_code_length_code_length_to_write, lit_len_code_lengths_to_write,
                                          dist_code_lengths_to_write, code_length_codes, lit_len_codes,
                                          distance_codes, offset + uncompressed_block_size == data_size,
                                          dynamic_compression_size, fixed_compression_size);
    // Free memory
    delete lit_len_tree;
    delete dist_tree;

    return res;
}

void
Deflate::Main::deflateFixed(const Byte* data, Deflate::Writer& writer, const CompressionInfo& compression_info,
                            const Memory& mem)
{
    // Avoid reallocation
    writer.data->reserve(writer.data->size() + compression_info.fixed_compression_size);

    writer.write_number(compression_info.is_last_block, 1); // BFINAL
    writer.write_number(1, 2); // BTYPE

    for (int i = 0; i < mem.num_symbols; ++i)
    {
        const Match* m = &mem.symbols[i];

        if (m->length() == 0)
            writer.write_code(fixed_lit_len_values_codes[m->val()], litLenFixedCodeLength(m->val()));
        else
        {
            const int length = m->length();
            const int distance = m->distance();
            const int length_code = m->length_code();
            const int dist_code = m->dist_code();

            // Length
            writer.write_code(fixed_lit_len_values_codes[length_code],
                              litLenFixedCodeLength(length_code)); // Length code
            writer.write_number(length - lit_len_code_to_length[length_code],
                                lengths_extra_bits[length_code]); // Length extra
            // Distance
            writer.write_number(dist_code, 5); // Distance code
            writer.write_number(distance - dist_code_to_dist[dist_code],
                                distances_extra_bits[dist_code]); // Distance extra
        }
    }

    writer.write_code(fixed_lit_len_values_codes[256], litLenFixedCodeLength(256)); // End of block
}

void
Deflate::Main::deflateUncompressed(const Byte* data, const int offset, Writer& writer,
                                   const CompressionInfo& compression_info)
{
    // Avoid reallocation
    writer.data->reserve(writer.data->size() + compression_info.uncompressed_size);

    writer.write_number(compression_info.is_last_block, 1); // BFINAL
    writer.write_number(0, 2); // BTYPE

    writer.write_curr_byte_if_not_empty(); // Padding

    writer.write_number(compression_info.uncompressed_size, 16); // LEN
    writer.write_number(~compression_info.uncompressed_size, 16); // NLEN

    // Write the block
    for (int i = 0; i < compression_info.uncompressed_size; ++i)
        writer.write_raw_byte(data[offset + i]);
}

int Deflate::Main::litLenFixedCodeLength(const int lit_len)
{
    if (lit_len <= 143 || lit_len >= 280) // 0-143 & 280-287
        return 8;
    else if (lit_len <= 255) // 144-255
        return 9;
    else if (lit_len <= 279) // 256-279
        return 7;

    throw std::runtime_error("Invalid literal/length");
}

void Deflate::Main::deflateDynamic(const CompressionInfo& compression_info, Deflate::Writer& writer, const Byte* data,
                                   const Memory& mem)
{
    // Avoid reallocation
    writer.data->reserve(writer.data->size() + compression_info.dynamic_compression_size);

    writer.write_number(compression_info.is_last_block, 1); // BFINAL
    writer.write_number(2, 2); // BTYPE
    writer.write_number(compression_info.provided_lit_len - 257, 5); // HLIT
    writer.write_number(compression_info.provided_dist_codes - 1, 5); // HDIST
    writer.write_number(compression_info.num_code_length_code_length_to_write - 4, 4); // HCLEN

    // Write the code length code lengths
    for (int k = 0; k < compression_info.num_code_length_code_length_to_write; ++k)
        writer.write_number(compression_info.code_length_codes[code_length_codes_order[k]].length, 3);

    // Write lit/len code lengths
    writeCodeLengths(writer, *compression_info.lit_len_code_lengths_to_write,
                     compression_info.code_length_codes);
    // Write distance code lengths
    writeCodeLengths(writer, *compression_info.dist_code_lengths_to_write,
                     compression_info.code_length_codes);

    // Write the compressed data
    writeCompressedData(writer, data, compression_info.offset, compression_info.uncompressed_size,
                        compression_info.lit_len_codes,
                        compression_info.dist_codes, mem);

    // Write end of block
    writer.write_code(compression_info.lit_len_codes[256].code,
                      compression_info.lit_len_codes[256].length);
}

void Deflate::Main::buildFixedHuffmanLitLenValuesCodes()
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

void
Deflate::Main::findBestMatch(const Byte* data, const Memory& mem, int& best_len, int& best_match, int& best_dist,
                             int& ind, int size)
{
    int curr_match = best_match;
    int chain_length = MAX_CHAIN_LENGTH;
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
    best_dist = ind - best_match;
}

void
Deflate::Main::savePreviousMatch(int match, int len, int dist, const Byte* data, int& ind, Deflate::Memory& mem, int& h)
{
    const int prev_length_code = lengthToLengthCode(len);
    const int prev_dist_code = distanceToDistanceCode(dist);

    mem.lit_len_frequency_table[prev_length_code]++;
    mem.dist_frequency_table[prev_dist_code]++;
    mem.symbols[mem.num_symbols++] = Match(data[ind - 1], len, dist,
                                           prev_length_code, prev_dist_code);

    // Add the hashes for the bytes in the match - Hashes for ind - 1 and ind are already added
    for (int i = 0; i < len - 2; ++i)
    {
        ind++;
        updateHash(h, data[ind + 2]);
        mem.prev[ind & window_mask] = mem.head[h];
        mem.head[h] = ind;
    }
}


Deflate::Main::CompressionInfo::CompressionInfo(int offset, int uncompressedSize, int providedLitLen,
                                                int providedDistCodes,
                                                int numCodeLengthCodeLengthToWrite,
                                                const std::vector<std::pair<int, int>>* litLenCodeLengthsToWrite,
                                                const std::vector<std::pair<int, int>>* distCodeLengthsToWrite,
                                                const HuffmanTree::Code* codeLengthCodes,
                                                const Deflate::HuffmanTree::Code* litLenCodes,
                                                const Deflate::HuffmanTree::Code* distCodes, bool is_last_block,
                                                const int dynamic_compression_size, const int fixed_compression_size)
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
          is_last_block(is_last_block),
          dynamic_compression_size(dynamic_compression_size),
          fixed_compression_size(fixed_compression_size)
{}

Deflate::Main::CompressionInfo::~CompressionInfo()
{
    delete[] code_length_codes;
    delete[] lit_len_codes;
    delete[] dist_codes;
    delete lit_len_code_lengths_to_write;
    delete dist_code_lengths_to_write;
}
