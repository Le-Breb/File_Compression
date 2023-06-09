#include "Huffman_Tree.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include <list>


Deflate::Huffman_Tree::Huffman_Tree(const std::unordered_map<int, int>& frequency_table)
{
    if (frequency_table.size() == 1)
    {
        root_ = new Node(-1, new Node(frequency_table.begin()->first, nullptr, nullptr), nullptr);
        return;
    }

    std::priority_queue<std::pair<int, Node*>, std::vector<std::pair<int, Node*>>, ascending> q;
    for (const auto& el : frequency_table)
        q.emplace(el.second, new Node(el.first, nullptr, nullptr));

    while (q.size() > 1)
    {
        std::pair<int, Node*> n1 = q.top();
        q.pop();
        std::pair<int, Node*> n2 = q.top();
        q.pop();
        int freq = n1.first + n2.first;
        Node* n = new Node(-1, n2.second, n1.second);
        q.emplace(freq, n);
    }

    root_ = q.top().second;
}

void Deflate::Huffman_Tree::add(Node* node, const int key, const int path, // NOLINT(misc-no-recursion)
                                const int path_length)
{
    if (!path_length)
    {
        node->key = key;
        return;
    }
    if (path & (1 << (path_length - 1)))
    {
        if (node->right == nullptr)
            node->right = new Node(-1, nullptr, nullptr);
        add(node->right, key, path, path_length - 1);
    }
    else
    {
        if (node->left == nullptr)
            node->left = new Node(-1, nullptr, nullptr);
        add(node->left, key, path, path_length - 1);
    }
}

Deflate::Huffman_Tree::Huffman_Tree(const std::map<int, std::pair<int, int>>& tree)
{
    root_ = new Node(-1, nullptr, nullptr);

    for (const auto p : tree)
        add(root_, p.first, p.second.first, p.second.second);
}

int Deflate::Huffman_Tree::read_key(Stream_Reader& reader) const
{
    const Node* current = root_;
    while (current->key == -1)
    {
        if (reader.read_bit())
            current = current->right;
        else
            current = current->left;
    }

    return current->key;
}

std::ostream& Deflate::operator<<(std::ostream& os, const Deflate::Huffman_Tree& huffman_tree)
{
    os << huffman_tree.root_;

    return os;
}

std::map<int, std::vector<int>> Deflate::Huffman_Tree::symbols_per_code_length() const
{
    std::map<int, std::vector<int>> res;
    Node* n = root_;
    std::queue<std::pair<Node*, int>> q;
    q.emplace(n, 0);
    while (!q.empty())
    {
        auto p = q.front();
        q.pop();
        if (p.first->key != -1)
            res[p.second].emplace_back(p.first->key);
        else
        {
            if (p.first->left != nullptr)
                q.emplace(p.first->left, p.second + 1);
            if (p.first->right != nullptr)
                q.emplace(p.first->right, p.second + 1);
        }
    }

    return res;
}

std::map<int, std::pair<int, int>> Deflate::Huffman_Tree::canonical_codes(const int max_bit_length) const
{
    std::map<int, std::vector<int>> symbols_per_bit_len = symbols_per_code_length();
    int overflow;
    do
    {
        overflow = false;
        for (auto& [bl, symbols] : symbols_per_bit_len)
        {
            if (bl <= max_bit_length)
                continue;
            int q = max_bit_length - 1;
            while (!symbols_per_bit_len.contains(q))
                q--;
            symbols_per_bit_len[q + 1].emplace_back(symbols_per_bit_len[q].back());
            symbols_per_bit_len[q].pop_back();
            if (symbols_per_bit_len[q].empty())
                symbols_per_bit_len.erase(q);
            symbols_per_bit_len[q + 1].emplace_back(symbols_per_bit_len[bl].back());
            symbols_per_bit_len[bl].pop_back();
            symbols_per_bit_len[bl - 1].emplace_back(symbols_per_bit_len[bl].back());
            symbols_per_bit_len[bl].pop_back();
            if (symbols_per_bit_len[bl].empty())
                symbols_per_bit_len.erase(bl);
            overflow = true;
            break;
        }
    } while (overflow);

    std::map<int, std::pair<int, int>> res;

    // Computing canonical codes and code lengths
    int code = -1;
    int prev_bit_len = 0;
    for (auto& [bit_length, symbols] : symbols_per_bit_len)
    {
        std::sort(symbols.begin(), symbols.end()); // Necessary for canonical coding
        for (const auto& s : symbols)
        {
            code = (code + 1) << (bit_length - prev_bit_len);
            res[s] = {code, bit_length};
            prev_bit_len = bit_length;
        }
    }

    /*std::map<int, std::pair<int, int>> original(res);
    // Fixing codes with bit length > max_bit_length
    for (auto& [bl, symbols] : symbols_per_bit_len)
    {
        if (bl > max_bit_length)
        {
            int q = max_bit_length - 1;
            while (symbols_per_bit_len[q].empty())
                --q;

            // Move one leaf down the tree
            res[symbols_per_bit_len[q].back()].first <<= 1;
            res[symbols_per_bit_len[q].back()].second += 1;
            // Move one overflow item as its brother
            res[symbols.back()].first = res[symbols_per_bit_len[q].back()].first + 1;
            res[symbols.back()].second = res[symbols_per_bit_len[q].back()].second;
            // Move the brother of the overflow item one step up the tree
            res[symbols.rbegin()[1]].first >>= 1;
            res[symbols.rbegin()[1]].second -= 1;

            // Update symbols_per_bit_len
            symbols_per_bit_len[q + 1].emplace_back(symbols_per_bit_len[q].back());
            symbols_per_bit_len[q].pop_back();
            symbols_per_bit_len[q + 1].emplace_back(symbols.back());
            symbols.pop_back();
            symbols_per_bit_len[bl - 1].emplace_back(symbols.back());
            symbols.pop_back();
            std::sort(symbols_per_bit_len[q + 1].begin(), symbols_per_bit_len[q + 1].end());
            std::sort(symbols_per_bit_len[q].begin(), symbols_per_bit_len[q].end());
            std::sort(symbols_per_bit_len[bl - 1].begin(), symbols_per_bit_len[bl - 1].end());
        }
    }

    if (original != res)
    {
        std::cout << std::endl;
        for (const auto& [symbol, p] : original)
            std::cout << symbol << " " << std::bitset<8>(p.first) << " " << p.second << std::endl;
        std::cout << std::endl;
        for (const auto& [symbol, p] : res)
            std::cout << symbol << " " << std::bitset<8>(p.first) << " " << p.second << std::endl;
    }*/


    return res;
}

