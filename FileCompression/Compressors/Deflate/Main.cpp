#include "Main.h"
#include "Huffman_Tree.h"
#include "Window.h"
#include "Match.h"
#include "Writer.h"
#include <exception>
#include <iostream>
#include <vector>
#include <list>

std::pair<bool, std::vector<char>> Deflate::Main::decompress_block(Stream_Reader& reader, Window& window)
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

std::vector<char> Deflate::Main::get_dynamic_huffman_data(Stream_Reader& reader, Window& window)
{
    std::vector<char> data;
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
            code_length_keys_paths[i] = {code_length_codes[i], code_length_code_lengths[i]};
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
            lit_len_codes_keys_paths[i] = {lit_len_codes[i], lit_len_code_lengths[i]};
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
            dist_codes_keys_paths[i] = {distance_codes[i], dist_code_lengths[i]};
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

std::pair<char*, int> Deflate::Main::deflate(const char* data, int size)
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
    std::map<int, std::pair<int, int>> lit_len_codes = lit_len_tree.canonical_codes();
    std::map<int, std::pair<int, int>> distance_codes = dist_tree.canonical_codes();
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
    std::map<int, std::pair<int, int>> code_length_codes = code_lengths_tree.canonical_codes();

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
    char* compressed_data = new char[writer.data.size()];
    for (int ind = 0; ind < writer.data.size(); ++ind)
        compressed_data[ind] = writer.data[ind];

    return {compressed_data, writer.data.size()};
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
        } while (!inflation.first);

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
                                            const int max_code_length)
{
    // Compute number of codes for each code length
    /// \brief bl_count[i] contains the number of codes of length i
    int bl_count[max_code_length + 1];
    int* code_length_codes = new int[num_codes];
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
        const int code_length_code_length = code_lengths[i];
        if (code_length_code_length > 0)
            code_length_codes[i] = next_code[code_length_code_length]++;
        else code_length_codes[i] = -1;
    }

    return code_length_codes;
}

auto a = "\n"
         "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer fringilla eros a neque sodales, vel feugiat tellus rutrum. Nullam viverra ut ligula in semper. Suspendisse in magna viverra, fermentum urna vitae, eleifend sapien. Morbi et accumsan ligula. Suspendisse enim sapien, volutpat at lacinia in, laoreet eu neque. Vestibulum sodales, lorem eu consectetur gravida, leo leo dictum lectus, non feugiat nulla augue ac nunc. Nulla et justo sed tortor pellentesque aliquam. Donec quis maximus leo. Suspendisse viverra purus aliquam molestie varius.\n"
         "\n"
         "Interdum et malesuada fames ac ante ipsum primis in faucibus. Vivamus quis erat elementum, fermentum neque vel, condimentum augue. Nulla vitae est nisl. Morbi nec purus rhoncus, elementum lectus eget, varius urna. Maecenas sed nunc erat. Praesent rhoncus viverra felis, at posuere purus condimentum sit amet. Proin accumsan nulla quis leo rhoncus commodo. Ut maximus non nisl eget volutpat. Fusce sodales mattis sem ac tristique. Proin vulputate vulputate nisl, et fringilla eros pretium a. Mauris eu dapibus sapien. Fusce commodo laoreet diam. Vestibulum vulputate hendrerit est feugiat tincidunt. Mauris porttitor nisl arcu, eget faucibus risus pharetra a.\n"
         "\n"
         "Maecenas sit amet nisl id nunc semper maximus. Ut sit amet arcu eget magna pharetra porta. Phasellus nec est ullamcorper, porta ligula vitae, luctus tortor. Nam a odio ultrices, egestas sapien in, aliquet augue. Mauris finibus ullamcorper velit, id gravida felis vehicula ultricies. Proin mollis semper est, ac tristique lorem facilisis vel. Aenean at vulputate enim, sed molestie risus. Vivamus placerat ac augue nec lacinia. Ut sed faucibus justo. Nunc nec tortor malesuada ante mollis tincidunt. Praesent sit amet enim sit amet erat volutpat sagittis.\n"
         "\n"
         "Proin arcu erat, feugiat a neque sit amet, molestie condimentum felis. Nulla vestibulum suscipit sem, vehicula sagittis ante gravida vel. Aenean tempor nulla est, quis efficitur lectus dignissim non. Duis semper diam enim, vel faucibus nisi consectetur eu. Integer lectus dolor, commodo et lorem et, suscipit dignissim felis. Proin non porttitor nisi. Cras ex est, interdum id diam at, convallis varius erat. Quisque efficitur ante at malesuada dictum. Morbi sed neque quis eros malesuada aliquam. Sed iaculis, purus sit amet fringilla tincidunt, ipsum libero semper mauris, ut ullamcorper justo dolor eget orci. Nunc efficitur finibus ex sed bibendum. Vivamus convallis semper nunc in posuere. Nunc sit amet lorem nulla.\n"
         "\n"
         "Nam ornare vulputate magna at varius. Sed vitae sodales ex, ac tempus nisi. Ut ac est id nisi aliquet mollis nec et nisi. Ut at lectus sed justo rhoncus tempus. Ut eget augue ac quam blandit pellentesque in id ante. Curabitur malesuada, sapien ac aliquet lacinia, lectus enim eleifend ipsum, quis semper sem dolor a sapien. Donec sit amet viverra quam, at mollis dolor. Pellentesque venenatis lacinia consequat.\n"
         "\n"
         "Sed lacinia eu felis et blandit. Pellentesque maximus urna metus, efficitur iaculis mi venenatis vel. Nam ut viverra dolor. Donec rutrum justo id ante imperdiet, in consectetur arcu vestibulum. Nam porta auctor tempor. Integer tincidunt convallis erat, quis feugiat arcu ultrices eu. Pellentesque ultrices aliquet risus eu pharetra. Mauris commodo imperdiet arcu. Sed quam ipsum, hendrerit sed pulvinar sit amet, venenatis id mi. Aenean gravida massa eget orci tempor, id interdum magna rhoncus. Integer tristique placerat neque.\n"
         "\n"
         "Fusce finibus mollis massa at efficitur. Donec ac semper nisi. Quisque placerat aliquam scelerisque. Curabitur non lacus non velit pellentesque pellentesque. Cras quis metus mollis, finibus odio id, sodales tortor. Integer vulputate pretium porta. Donec posuere elit mauris. Integer sit amet nibh libero. Sed iaculis nibh non nisi ornare fringilla vitae a felis. Mauris cursus blandit nisi non tincidunt.\n"
         "\n"
         "Sed auctor tempus mollis. Nam nec euismod quam. Duis gravida sapien nec erat finibus, ut ullamcorper enim viverra. Nulla porta egestas metus ut finibus. Suspendisse urna eros, porta ac dolor nec, iaculis placerat sapien. Nunc finibus enim at euismod tristique. In pulvinar quis nisl nec auctor. Ut luctus velit nec lectus interdum sollicitudin. Vivamus sem velit, volutpat sed libero sit amet, pulvinar ultrices nibh. Fusce elementum, ipsum non sodales tincidunt, ex leo scelerisque nisl, tristique vestibulum magna justo ac lorem. Integer vel sapien ante. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Aliquam ut fermentum tellus. Aliquam tincidunt ut eros dignissim molestie.\n"
         "\n"
         "Nullam a consequat sapien. Quisque non mattis diam, sit amet vulputate velit. Donec sed augue sed massa iaculis semper. Pellentesque quam sem, iaculis at consectetur ac, porttitor ut felis. Integer mi ipsum, molestie vel justo in, hendrerit porta mauris. Sed sollicitudin pulvinar nibh, convallis varius elit tempus in. Sed in convallis nibh. Curabitur vel diam elit. Morbi porttitor porta consequat.\n"
         "\n"
         "Pellentesque pellentesque massa nec velit mattis, nec sollicitudin augue dictum. Donec non pharetra nulla. Aliquam luctus felis et ipsum convallis, porta sagittis leo eleifend. Integer vestibulum velit turpis, a consequat dui aliquet at. Nulla non ante leo. Ut rutrum arcu cursus, malesuada tortor ac, pellentesque magna. Mauris ultrices risus et nisl porttitor, nec suscipit neque euismod. Nullam sollicitudin suscipit diam, eu dictum est placerat non. Cras fringilla nulla est, non consectetur dolor tincidunt ac.\n"
         "\n"
         "Aliquam eu leo porttitor, consequat nisi id, dignissim orci. Nulla facilisi. Etiam vulputate, sem id imperdiet mattis, lorem sem faucibus urna, id varius nisi enim non felis. Praesent congue dolor vel arcu euismod malesuada. Donec aliquet commodo pulvinar. Mauris sit amet mauris nec justo pretium iaculis. Quisque bibendum nisl erat, ac ultrices mi consectetur nec. Nulla eu augue purus. Nam finibus ac arcu ac rutrum. Pellentesque tincidunt dictum tortor, in pharetra ex elementum pharetra. Donec vel velit purus. Praesent pretium libero urna, et rhoncus orci varius non. Cras tempor magna sodales sodales tincidunt.\n"
         "\n"
         "Interdum et malesuada fames ac ante ipsum primis in faucibus. Praesent porta ligula non rutrum sagittis. Praesent ullamcorper sapien tempor, pulvinar enim et, tincidunt nisi. Nullam imperdiet, nunc et dignissim tincidunt, eros enim sodales ipsum, nec vehicula dui libero nec quam. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas ut laoreet arcu, eu cursus ex. Morbi placerat placerat purus et bibendum. Sed pharetra dolor a lectus molestie elementum. Morbi in pellentesque sapien. Quisque quis odio urna.\n"
         "\n"
         "Duis imperdiet ut nisl ultrices scelerisque. Morbi ut nunc condimentum, blandit sem tempus, sollicitudin diam. Pellentesque leo est, ultrices nec erat at, mattis volutpat erat. Aenean commodo quis velit sit amet consectetur. Phasellus congue hendrerit pharetra. In tincidunt, lectus fringilla imperdiet semper, nulla erat placerat ipsum, nec dignissim eros nisl quis massa. Vestibulum id pretium risus. Aliquam blandit interdum felis nec tincidunt. Cras pharetra consectetur feugiat. Praesent eu enim aliquet augue congue hendrerit. Nulla ultricies, neque eu ultrices pellentesque, risus ligula gravida eros, sed cursus turpis felis finibus libero. Ut sit amet mollis magna, vitae faucibus elit. Donec leo elit, eleifend ut mollis id, tristique volutpat sapien. Cras et pellentesque augue.\n"
         "\n"
         "In lorem neque, bibendum in pulvinar nec, tempor ut est. Donec sit amet urna quis augue pulvinar pulvinar sit amet eu purus. Mauris id sollicitudin nulla. Curabitur convallis neque ex, non dictum urna aliquam ac. Nam viverra volutpat ornare. Donec magna ex, pellentesque a felis in, pretium dignissim lorem. Quisque malesuada mi libero, nec tristique lacus tempor at. Integer urna neque, aliquet et augue nec, aliquam varius augue. Nunc finibus posuere nisl, eu consequat orci consequat at. Cras ut arcu id dui mattis sagittis non et turpis. Nulla nec tellus posuere, vehicula velit ut, bibendum leo. Duis tristique sagittis malesuada. Proin a augue quis metus sagittis rhoncus. Sed lectus turpis, aliquet a finibus sed, fermentum at mauris. Vivamus eget dapibus felis. Maecenas vehicula ut diam quis laoreet.\n"
         "\n"
         "Duis a mi volutpat, maximus neque a, vestibulum urna. Donec at semper nunc. Morbi molestie magna justo, id semper turpis suscipit non. Morbi lorem lectus, semper ac iaculis sit amet, iaculis ut nisi. Donec non bibendum mauris. Nunc scelerisque malesuada dapibus. Aenean libero turpis, sagittis ac odio id, eleifend tincidunt libero. Vestibulum non mi eget sem maximus vulputate bibendum sagittis quam. Quisque ac ornare nisl. Donec eleifend enim eu elit pretium, eu vestibulum mi fringilla.\n"
         "\n"
         "Cras quis dapibus leo, in lobortis sem. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Praesent sodales eget erat id sollicitudin. Praesent bibendum augue non nisl euismod, et viverra magna sollicitudin. Sed vitae cursus elit. Cras consequat nec tellus et blandit. Nulla vulputate ipsum consectetur quam sollicitudin dignissim eu et arcu. Mauris tempus felis ut elit fringilla consequat. Vivamus et quam erat. Aliquam ut pellentesque metus, in venenatis lacus. Mauris dolor neque, congue vitae risus ut, molestie faucibus elit. Sed quis velit ut eros congue pharetra et sed enim. Maecenas nec urna diam. Phasellus bibendum libero massa, ut rutrum nisi placerat posuere. Vivamus vitae imperdiet leo, vitae eleifend ipsum.\n"
         "\n"
         "Pellentesque nulla purus, sollicitudin eu erat ultrices, viverra pulvinar leo. Sed tellus sapien, semper ut mollis ac, consequat nec libero. Sed imperdiet vitae turpis vitae luctus. Aenean quis felis pulvinar velit bibendum ultricies ac ac nisl. Etiam quis placerat ipsum. Nam vel tristique nibh. Praesent tincidunt ultricies elit non dignissim.\n"
         "\n"
         "Duis nec nisl sem. Nullam euismod lectus ut rutrum condimentum. Quisque non molestie purus, et suscipit risus. Nunc mi dui, aliquam a dui non, tincidunt aliquet neque. Suspendisse convallis volutpat eros ac volutpat. Nullam purus lacus, posuere et rhoncus non, posuere sit amet diam. Pellentesque posuere placerat finibus. Integer risus urna, lacinia sed libero nec, porta vestibulum leo. In volutpat suscipit arcu vitae tempus. Maecenas non lectus leo. Fusce leo ante, iaculis ac interdum sit amet, porttitor id dui. Phasellus commodo elit urna, eu feugiat magna feugiat ac. In id condimentum elit. Vivamus at eros nec nibh rhoncus facilisis sollicitudin eget elit. Phasellus laoreet arcu et quam gravida dapibus. Vestibulum tincidunt ex quis cursus maximus.\n"
         "\n"
         "Donec eget rhoncus ipsum. Cras efficitur nec neque ac pellentesque. Pellentesque aliquam metus non massa porttitor ultricies. Nam purus ligula, venenatis et laoreet vel, vulputate ut libero. Interdum et malesuada fames ac ante ipsum primis in faucibus. Fusce a pellentesque massa. Vivamus vel ornare erat. Aenean in vehicula lorem, nec volutpat libero. Praesent vitae molestie massa. Aenean ipsum velit, condimentum vel lobortis pulvinar, cursus sed mauris.\n"
         "\n"
         "Mauris vitae eleifend massa, eget porta sem. Sed egestas maximus dolor, eu efficitur risus consectetur pulvinar. Aliquam erat volutpat. Praesent sit amet odio facilisis, bibendum turpis nec, dignissim tortor. Praesent feugiat, nisl sed pellentesque aliquam, risus mi sollicitudin tortor, non mollis neque nisi ac metus. Mauris eget cursus dolor. Maecenas non velit ante. Aenean justo nunc, commodo eget semper vel, aliquet vitae ex. Praesent sodales tempor aliquam. Nullam eget lorem molestie quam blandit semper. Curabitur volutpat lorem eu molestie aliquam. Quisque accumsan vel diam non commodo. In hac habitasse platea dictumst. In vitae nisi tristique, ornare tortor at, dictum purus. In lacinia ex ac lorem cursus placerat.\n"
         "\n"
         "Etiam facilisis ante vel mauris porttitor tristique. Nulla sit amet tellus nec neque finibus ultrices in sit amet metus. Mauris commodo nisl sit amet lacus consequat, ut dapibus libero scelerisque. Sed eget ex eget ex tempus auctor. Nam massa lorem, viverra vel ultrices in, pellentesque sit amet purus. Ut viverra dui nisi, quis dignissim enim pellentesque eu. Vestibulum finibus feugiat purus at tincidunt. Sed in aliquet arcu. Sed varius enim nec sem eleifend, at rhoncus urna sodales. Etiam sed metus dignissim, elementum odio vel, auctor mi. In elementum convallis luctus. Vivamus ac dictum est. Phasellus vehicula feugiat magna, id cursus neque gravida euismod.\n"
         "\n"
         "Maecenas porta risus ligula, at porta nisi feugiat id. Morbi sed sem vitae dui imperdiet tristique. Nulla placerat justo nunc, sit amet dictum erat ornare eu. Mauris maximus vitae sapien nec lacinia. Maecenas scelerisque venenatis neque vitae scelerisque. Donec tincidunt scelerisque purus id mattis. Etiam massa massa, ullamcorper non metus sed, fringilla commodo felis.\n"
         "\n"
         "Cras scelerisque urna in erat rutrum gravida. Curabitur lorem ex, laoreet eget felis in, pretium posuere mi. Etiam vulputate sem nec odio fringilla vestibulum. Donec at tincidunt mi, eget euismod leo. In porta venenatis imperdiet. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Vestibulum massa mi, ornare eu quam sit amet, mollis convallis magna. Cras ultrices hendrerit ipsum, eget pulvinar erat commodo ut. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Proin placerat purus tellus, ut tristique est aliquet euismod. Fusce non arcu quis tellus feugiat dapibus eu sodales massa. Nunc tempus ipsum et mi pellentesque efficitur. Ut dapibus nisi a suscipit cursus.\n"
         "\n"
         "Nullam porttitor pellentesque volutpat. Nullam nec laoreet elit, sit amet consequat dolor. Nulla lorem nunc, fermentum ac faucibus id, malesuada et arcu. Nulla facilisi. Maecenas aliquam vitae orci cursus elementum. Sed varius mollis nisl non venenatis. Morbi at leo magna. Lorem ipsum dolor sit amet, consectetur adipiscing elit. In convallis rutrum tempor. Curabitur id sodales ante. Donec lobortis id ipsum quis congue. Nulla facilisi. Ut viverra, sapien a egestas varius, risus dui consectetur nisi, quis vulputate lorem purus sed neque. Curabitur hendrerit lorem vitae tempor congue. Phasellus elementum nisi ut erat scelerisque, nec pretium arcu aliquam.\n"
         "\n"
         "Sed vehicula tempus quam. Aenean vestibulum dui enim, sed posuere magna porta in. Donec pulvinar pellentesque massa in dictum. Vivamus mauris justo, volutpat non luctus vitae, laoreet eu leo. Aliquam eget eros nulla. Maecenas varius viverra nisi, non tincidunt erat consectetur sed. Praesent lacinia lorem vel leo dapibus pellentesque. Morbi non sollicitudin turpis, eget molestie mi. Duis mollis lobortis leo vitae tempus. Vivamus efficitur tortor et felis rutrum sagittis. Sed vitae arcu sit amet massa posuere facilisis eget at odio. Sed pharetra sollicitudin sollicitudin.\n"
         "\n"
         "Etiam ut finibus nunc, at porttitor erat. Ut molestie sapien id tortor fringilla, nec malesuada tortor euismod. Donec vel convallis massa, vitae dapibus diam. Quisque lacinia consequat dignissim. Donec vitae mi non nulla auctor dapibus. Duis vel sagittis erat. In hac habitasse platea dictumst. Praesent sodales ultricies ultrices. Curabitur auctor est at est congue, a mattis mi ornare. Fusce rutrum ut purus at cursus. Aliquam bibendum mattis urna non iaculis. Vestibulum ullamcorper ante vitae nunc auctor lobortis. Sed ac libero condimentum, efficitur ex porttitor, pulvinar velit. Aliquam eget nibh at quam commodo lacinia ut non odio. Praesent pretium laoreet lorem et bibendum. Nunc pulvinar sem venenatis, euismod diam quis, semper ante.\n"
         "\n"
         "Interdum et malesuada fames ac ante ipsum primis in faucibus. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Maecenas eget lectus mauris. Interdum et malesuada fames ac ante ipsum primis in faucibus. Sed pulvinar eget purus posuere pretium. Integer interdum bibendum sem quis consequat. Cras eros velit, fringilla interdum vehicula venenatis, iaculis porta tortor. Cras ut ipsum sed lacus aliquam vulputate. Sed in efficitur leo.\n"
         "\n"
         "Quisque sit amet vehicula quam. Vivamus massa purus, varius vel dolor sed, aliquet eleifend risus. Quisque finibus arcu quam, non vulputate mi porttitor at. Sed lacus odio, sollicitudin et porttitor quis, aliquam ac libero. Vivamus luctus eros ut imperdiet ultrices. Suspendisse et tristique erat. Etiam ut odio ex. Nullam porta lacus tortor, placerat pharetra lectus ultrices ac. Donec euismod felis metus. Vestibulum sagittis imperdiet ipsum ac lacinia. Sed et mauris at quam maximus maximus. Pellentesque suscipit tempus dolor, ut hendrerit dolor pretium ac. Curabitur pellentesque ornare molestie. Suspendisse tortor ligula, blandit vel ultricies a, ullamcorper convallis sapien. Cras at hendrerit velit, id tincidunt justo.\n"
         "\n"
         "Proin fringilla justo dictum venenatis elementum. Donec fermentum erat sit amet accumsan ultrices. Quisque maximus nulla porttitor semper finibus. Duis quis purus eros. Nam porta, ante sit amet elementum sollicitudin, nisi odio tempor dui, at condimentum quam velit ut nisi. Vestibulum mollis scelerisque libero nec bibendum. Cras congue hendrerit porta. Nam non leo odio. Fusce hendrerit in sem at bibendum.\n"
         "\n"
         "Vivamus fermentum felis ut enim vestibulum placerat. Ut ultricies sapien vitae elit placerat, quis semper lorem malesuada. Curabitur eget volutpat dui. Maecenas faucibus mollis tristique. Duis vehicula massa at rhoncus molestie. Morbi consequat ante lectus, lacinia imperdiet arcu elementum volutpat. Sed sodales imperdiet dui ut tempus. Vestibulum auctor tristique interdum. Quisque gravida iaculis ultricies. Sed eu felis quam. Morbi dapibus accumsan varius. Pellentesque ornare, elit nec commodo rutrum, nisl tellus placerat diam, a maximus dui magna vitae lorem. Curabitur elementum libero nisl, dapibus accumsan enim lobortis et. Etiam hendrerit finibus nibh, non condimentum urna venenatis et. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus.\n"
         "\n"
         "Suspendisse id nibh convallis, ullamcorper nisi a, egestas nulla. Nam viverra nisl nunc, eu faucibus est molestie vitae. Nullam massa ipsum, condimentum vitae maximus aliquet, imperdiet eget nibh. Quisque non iaculis ex. In egestas vehicula urna, nec viverra enim pellentesque sed. Nullam commodo mauris vel nisl egestas, vel vehicula lacus vehicula. Pellentesque ac erat quis nunc cursus vestibulum id eu arcu.\n"
         "\n"
         "Ut et diam semper, rutrum ligula a, facilisis leo. Phasellus risus enim, rutrum ac tincidunt ultricies, egestas vel erat. Vestibulum aliquam augue sed neque imperdiet viverra. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Nam id mollis felis. Aenean pulvinar vulputate lorem, ut condimentum quam accumsan vel. Maecenas pulvinar dui nec tortor porttitor ornare. Nam vulputate id erat in tempus. Nam sit amet est cursus, gravida nibh eget, tincidunt tortor. Morbi nec placerat leo. Etiam tempor aliquam neque, quis vestibulum nibh luctus vitae. Sed in pretium eros, in auctor enim. Nunc nec urna maximus, scelerisque enim sed, maximus orci. Pellentesque risus urna, dapibus id venenatis eu, porttitor eu massa. Suspendisse luctus metus vitae tincidunt viverra. Vivamus quis libero in lectus tincidunt iaculis in eget arcu.\n"
         "\n"
         "Mauris odio dolor, luctus eget mi in, mollis aliquam lectus. Praesent venenatis odio id lectus ullamcorper egestas. Integer tempor dolor vel sollicitudin sollicitudin. Sed orci ligula, varius in dolor in, fringilla aliquam lectus. Ut interdum orci in augue elementum, in sodales dui tempor. Nulla lacinia mollis nunc in sagittis. Proin ante risus, euismod eu ex ac, facilisis tincidunt diam. Ut quis posuere nibh.\n"
         "\n"
         "Nunc quis tincidunt quam. Sed in laoreet turpis. Quisque quis mauris varius, luctus nibh sit amet, viverra leo. Phasellus venenatis massa turpis, at viverra velit gravida eu. Ut commodo odio nec pulvinar lobortis. Phasellus non fermentum mi. Aenean porttitor, sapien non blandit malesuada, ligula felis gravida ante, vel vulputate ex nulla porta est. Phasellus quis vestibulum ipsum. Fusce dictum felis augue, dapibus fermentum metus blandit id.\n"
         "\n"
         "Etiam lobortis finibus varius. Vestibulum id ultricies leo. Donec arcu ipsum, tincidunt non massa eget, venenatis congue massa. Maecenas nec tempus purus, sit amet sodales eros. Aenean vel lorem eget urna maximus ullamcorper vitae ut libero. Nullam nec mi risus. Vestibulum vitae metus luctus, consectetur lorem et, consectetur urna. Aliquam laoreet sem ac nibh finibus feugiat ac sit amet justo. Nullam consectetur sem nec mi semper feugiat. Vivamus pretium, quam sed suscipit interdum, magna ipsum bibendum sapien, quis tincidunt mi augue in felis. Aliquam viverra purus sed neque suscipit fermentum. Integer sed consectetur lectus. Ut cursus lacinia ipsum et ultricies.\n"
         "\n"
         "Integer bibendum sapien non scelerisque lobortis. Praesent varius eleifend fringilla. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fusce vulputate lacinia lacinia. Pellentesque sit amet massa risus. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Mauris vulputate blandit risus, fermentum feugiat elit sodales scelerisque. Vestibulum non aliquet augue, quis scelerisque dui. Donec sed felis rhoncus, volutpat enim vitae, faucibus ex. Vestibulum feugiat sollicitudin porttitor. Nulla quam metus, vulputate nec finibus pharetra, dapibus ac augue. Vivamus et nibh elementum, fringilla nisl vel, egestas lorem.\n"
         "\n"
         "Sed sit amet finibus odio. Aenean semper aliquam blandit. Quisque a urna nunc. Curabitur posuere, nibh quis tempor hendrerit, ante mauris dictum mauris, quis convallis felis ligula non mauris. Vestibulum eu neque auctor, dapibus nisl ac, tristique odio. Sed id metus et enim congue lobortis. Cras sapien leo, dictum at pulvinar at, imperdiet ut sem. Nulla cursus neque et tellus bibendum, eu vulputate tellus mollis. In viverra dui quis sem posuere, bibendum faucibus lorem rutrum. In diam massa, aliquet non mollis scelerisque, dictum id nunc. Nullam quis elementum lacus. Fusce ut interdum purus. Nunc malesuada nisl non nunc maximus, nec facilisis eros sagittis. Pellentesque varius volutpat augue, scelerisque lobortis ex faucibus a. Nam eget quam a massa dignissim imperdiet.\n"
         "\n"
         "Ut id tincidunt odio, quis condimentum ante. Integer commodo arcu lectus, sit amet eleifend sapien iaculis in. Quisque sed neque elementum, accumsan ante in, commodo arcu. Integer ut purus nunc. Praesent quis magna eu purus congue tempus. Etiam quis semper odio. Curabitur consequat, ligula id luctus tempus, sapien purus eleifend lorem, eu mollis massa erat et lectus. Sed sit amet neque finibus, suscipit lectus vitae, consectetur augue. Cras id vulputate mi. Nulla aliquam ornare ex, in molestie turpis blandit sit amet. Donec purus erat, luctus eget augue sit amet, malesuada varius sapien. Nam accumsan nulla sed magna ultricies fermentum. Etiam euismod eget orci non porttitor.\n"
         "\n"
         "Curabitur euismod sagittis blandit. Suspendisse eros lectus, interdum ornare ligula vel, auctor vehicula sapien. Ut iaculis condimentum tortor sed malesuada. Phasellus ut semper est. Sed interdum velit non aliquam volutpat. Phasellus enim purus, gravida ac convallis id, viverra ut ipsum. Donec bibendum elit neque. Duis ac vehicula velit. Quisque ut varius felis, at bibendum quam. Nullam in orci lobortis, aliquet est tempor, ornare tortor. Vivamus molestie massa et urna elementum posuere eget vel libero. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Duis finibus, dolor ut auctor blandit, nisi nibh gravida mauris, quis fringilla purus sapien volutpat nibh.\n"
         "\n"
         "Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Maecenas molestie quam tortor, laoreet lacinia ipsum laoreet eu. Nulla facilisi. Interdum et malesuada fames ac ante ipsum primis in faucibus. Sed ac aliquam nunc. Suspendisse mollis nulla eu sem aliquet, at sagittis metus placerat. In vitae dui eget tortor finibus rutrum vel ac ligula. Donec et arcu ac lacus rhoncus venenatis at sit amet dui. Vestibulum vulputate sed mauris at egestas. Aliquam eget elit at lorem semper pulvinar. Duis laoreet ultricies libero sit amet semper. Suspendisse lobortis quam nec nisl vulputate, vitae lobortis leo tincidunt.\n"
         "\n"
         "Curabitur vulputate ultricies elementum. Aliquam sodales, metus et condimentum accumsan, quam tortor rhoncus nunc, id rutrum neque neque at lectus. Pellentesque ac molestie ligula, ac scelerisque mauris. Cras dignissim, odio a suscipit maximus, risus augue fermentum massa, eu tempus lorem nulla vel sapien. Aliquam in egestas risus. Donec ac nibh erat. Quisque nibh eros, gravida blandit nunc volutpat, ornare ornare dui. Morbi a ligula sed erat cursus ultricies sit amet nec augue. Morbi pretium fringilla interdum. Pellentesque vitae sapien sed mauris commodo faucibus. Duis in tincidunt massa, ac eleifend lacus. Aenean accumsan viverra ante a eleifend.\n"
         "\n"
         "Aliquam accumsan sit amet mauris id facilisis. Nulla sed lacus nec risus molestie feugiat. Mauris feugiat mauris diam, quis bibendum tellus lobortis quis. Nunc placerat ante in lorem gravida facilisis. Proin venenatis ligula eu justo condimentum bibendum. Donec ultrices placerat felis, quis fringilla enim scelerisque eu. Maecenas sollicitudin ut risus sed molestie. Fusce nec eros mi. Sed quis eleifend nunc. Donec sodales vitae ante suscipit lobortis.\n"
         "\n"
         "Integer luctus est in aliquet porta. Vivamus sed leo lobortis, pulvinar lectus molestie, tempus ex. Aenean libero diam, scelerisque at nisl vel, aliquam mattis arcu. Mauris egestas, massa sed condimentum aliquet, felis ligula fermentum lorem, ut commodo erat tortor eu nunc. Mauris ac convallis nisi. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Suspendisse mattis efficitur pharetra. Nulla congue, augue at finibus scelerisque, augue est blandit justo, ac fringilla orci nibh vitae lacus.\n"
         "\n"
         "Integer laoreet purus dictum, viverra neque sed, efficitur tortor. Morbi hendrerit varius ante, in ultrices urna fermentum id. Donec ac eros et est mollis gravida in vel risus. Curabitur tellus ante, tristique non congue at, molestie id leo. Sed suscipit turpis eget velit congue pretium. Nulla facilisi. Fusce fringilla ante at arcu convallis semper eu vitae metus. In mattis nibh facilisis gravida congue. Donec blandit, dui id aliquet mollis, tortor elit pretium nunc, id aliquam nisi metus at odio. Nulla facilisi. Nulla facilisi. Maecenas pulvinar sit amet magna fringilla consequat.\n"
         "\n"
         "Duis vitae leo nulla. Nunc quis dignissim nunc, nec pellentesque dui. Maecenas efficitur in enim non pellentesque. Donec fringilla pharetra maximus. Sed feugiat hendrerit pretium. Donec consectetur ex in arcu ultricies interdum. Sed elit nibh, scelerisque vitae tortor venenatis, accumsan aliquet dolor.\n"
         "\n"
         "Mauris faucibus porta nisl. Ut neque leo, molestie at nulla eget, malesuada pharetra tortor. Nulla vestibulum ligula eu est efficitur consectetur. In feugiat tempus neque eget consequat. Phasellus at gravida nulla, non eleifend lacus. Praesent eu sem vehicula, posuere eros nec, maximus nisl. Aenean in lectus ac nulla pretium interdum vitae vitae lectus. In hac habitasse platea dictumst. Aenean rhoncus et turpis id rutrum. Mauris ac varius elit.\n"
         "\n"
         "Etiam libero quam, varius ac tellus ut, ultrices facilisis neque. Quisque eget sapien scelerisque turpis tempor blandit at sit amet mauris. Maecenas sit amet dictum ex, eu feugiat dui. Aliquam finibus sapien id volutpat malesuada. Nulla facilisi. Suspendisse pulvinar massa in odio scelerisque, nec sagittis odio rutrum. Pellentesque vitae sagittis ligula. Aliquam erat volutpat. Nulla placerat suscipit nisl, sit amet tristique ante fringilla vel. Maecenas imperdiet quam egestas, mattis tortor ac, mollis libero. Phasellus in condimentum nulla, nec fringilla turpis. Praesent dictum massa in molestie tincidunt. Nulla semper quam id tristique malesuada. Etiam suscipit eros finibus ligula ornare rhoncus. Donec ultricies libero sem, non hendrerit nibh sagittis a.\n"
         "\n"
         "Nam fringilla in ipsum vel ullamcorper. In scelerisque diam at dolor rhoncus laoreet. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Etiam tempor ante non arcu congue, a tristique ante efficitur. Suspendisse eget justo facilisis, consequat dolor vitae, cursus ex. Nulla eget velit justo. Praesent dictum ornare est, volutpat vestibulum arcu scelerisque non. Fusce sit amet convallis arcu. Vivamus ornare odio vel sem sagittis tempus. Donec sollicitudin nisi eu eros pellentesque, non ultricies tellus pharetra. Nulla at est dolor.\n"
         "\n"
         "Duis in lacus eu elit porta volutpat nec a sem. Nulla ut sodales metus. Nunc fringilla dui lorem, ut fermentum mauris euismod vel. Sed rhoncus auctor velit sed feugiat. Sed eget leo eu dolor pretium porta. Proin nec ante ultrices, fringilla arcu et, vulputate arcu. Vestibulum eget cursus mauris.\n"
         "\n"
         "Sed non quam at felis sollicitudin mattis. Cras blandit odio eget tincidunt pulvinar. Donec finibus rhoncus velit, a gravida metus tincidunt ac. Pellentesque pellentesque fermentum ipsum eu convallis. Suspendisse feugiat arcu vitae magna rhoncus cursus. Sed eget mauris et nisl tristique condimentum. Integer dui sem, luctus at leo in, fringilla volutpat justo. In a ornare ipsum. Curabitur ex magna, consequat quis sodales quis, scelerisque sit amet ipsum. Integer tincidunt, quam sit amet congue scelerisque, diam ex luctus enim, ac eleifend nunc augue sed tortor. Fusce justo lectus, varius eget arcu et, lobortis ultricies quam.\n"
         "\n"
         "Curabitur sed scelerisque mi, eu sodales leo. Nulla pellentesque pretium enim, sit amet vestibulum tellus lacinia ut. Aliquam eleifend pellentesque metus, ac mollis elit tempus sit amet. Nam ac ligula mi. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Suspendisse viverra est velit, in tincidunt sem sollicitudin id. Aenean sodales leo ut mattis vestibulum. Vestibulum a interdum mi, a auctor nunc. Proin lobortis mattis augue a bibendum. Phasellus dignissim, erat ac gravida luctus, mauris nibh ullamcorper eros, vitae cursus tortor ante a purus. Mauris eu maximus purus.\n"
         "\n"
         "Nam condimentum mauris leo, nec placerat nunc eleifend id. In hendrerit neque sit amet lorem ornare congue. Etiam ac tristique enim. In sed libero finibus, blandit erat sed, mattis lacus. In non risus ligula. Phasellus vel posuere nisi. Mauris quis nunc ligula. Nunc non ullamcorper ligula, vel interdum sapien. Praesent sed pellentesque libero, non aliquet urna. Etiam tempor nisl nisi, vitae porttitor ante tempus vulputate. Sed eget purus sed lacus accumsan rutrum. Maecenas a nibh nec nulla tincidunt facilisis sed eu dui. Duis id nisl dictum, congue sapien at, vulputate mauris. Nunc aliquet dignissim dui et cursus. Praesent porta, tellus ac sodales venenatis, leo arcu fringilla elit, eget laoreet dolor ipsum in elit. Aenean vitae orci erat.\n"
         "\n"
         "In at molestie est, nec semper leo. Suspendisse viverra viverra lectus vitae porttitor. Mauris porttitor sit amet justo a faucibus. Proin ultricies, nulla in porttitor dictum, diam sapien condimentum turpis, quis vestibulum mi leo vitae magna. Vivamus congue condimentum velit, non tristique mi tincidunt quis. Proin blandit lacinia lectus, quis hendrerit ipsum. Vestibulum tincidunt sem sit amet ex tristique vestibulum. Sed sit amet luctus nibh. Nulla facilisi.\n"
         "\n"
         "Nullam vitae arcu id enim elementum condimentum. Ut lobortis arcu vel nulla volutpat accumsan. Nullam ultrices, velit id sodales euismod, arcu urna porttitor velit, id mattis nisi mi eu enim. Sed feugiat pellentesque quam et facilisis. Sed maximus mauris in lorem vulputate, in hendrerit lectus molestie. Vestibulum lectus mauris, condimentum vitae ultrices in, cursus non eros. Mauris id scelerisque augue, ut sagittis enim. Nullam magna diam, faucibus a pretium ultricies, aliquet a velit. Fusce non dolor ac mi dapibus luctus sed feugiat velit. Sed sodales erat quis finibus fermentum. Proin ut sapien sed nunc ornare semper eget et dolor. Nulla tempus rhoncus lacus, vel fringilla nibh pulvinar pharetra. In ac nisi quis lacus tempus molestie. Cras consectetur odio et convallis bibendum.\n"
         "\n"
         "Cras nec nulla mattis, convallis libero quis, vulputate lorem. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Aliquam tristique tincidunt arcu, eu vestibulum mi porttitor sit amet. Curabitur feugiat, ligula a blandit malesuada, mi ante semper ligula, ut iaculis ante ligula ac ante. Vestibulum auctor leo quis metus ornare ultrices. In nunc nibh, porta a porta ac, accumsan quis odio. Sed iaculis elit at elementum scelerisque. Sed non tristique ante. In accumsan condimentum lacus, quis facilisis urna laoreet et. Duis pharetra feugiat purus, non commodo arcu vulputate a.\n"
         "\n"
         "Sed ultricies, odio nec efficitur mollis, libero risus commodo tortor, sit amet auctor urna felis quis leo. Donec sed tellus fermentum, venenatis ante sed, dignissim sem. Proin eu molestie sem. Vestibulum in sem vel ligula scelerisque tempus quis et lacus. Quisque luctus imperdiet iaculis. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Sed rhoncus lacus in mauris molestie, sit amet tempus risus dapibus.\n"
         "\n"
         "Sed in convallis sapien. Aliquam nec laoreet sapien. Morbi vitae ultricies quam. In sit amet pharetra lacus, vel facilisis nisl. Sed vitae ultrices neque. Quisque ut est gravida, eleifend lectus vel, blandit enim. Ut mauris eros, hendrerit sit amet nisi in, condimentum finibus orci. Duis efficitur magna sit amet lacus scelerisque ornare. Donec tristique, dolor sed dapibus mollis, tellus felis scelerisque lacus, quis euismod ante ipsum vel elit. Vestibulum eu ultrices justo. Curabitur euismod, eros ut dignissim fringilla, nunc dolor.";

std::vector<char> Deflate::Main::read_dynamic_huffman_data(Stream_Reader& reader, Huffman_Tree& lit_len_tree,
                                                           Huffman_Tree& dist_tree, Window& window)
{
    std::vector<char> data;
    int lit_len = lit_len_tree.read_key(reader);
    int co = 0;
    int d = 0;
    while (lit_len != 256)
    {
        // Literal
        if (lit_len <= 255)
        {
            data.push_back(static_cast<char>(lit_len));
            window.add(static_cast<char>(lit_len));
            if (a[co++] != static_cast<char>(lit_len))
                throw std::runtime_error("Invalid literal!");
        }
        else if (lit_len <= 287) // Length
        {
            d++;
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
                if (a[co++] != c)
                    throw std::runtime_error("Invalid match!");
            }
        }
        else
            throw std::runtime_error("Invalid dynamic huffman code!");

        lit_len = lit_len_tree.read_key(reader);
    }

    return data;
}

std::list<Deflate::Match*> Deflate::Main::find_matches(const char* data, int size, int offset)
{
    std::list<Match*> matches;
    std::unordered_map<int, std::list<int>> q;
    bool matchOnPreviousByte = false;
    int ind = 0;
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
