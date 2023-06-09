#pragma once

#include <map>
#include <ostream>
#include <vector>
#include <unordered_map>
#include "Main.h"

namespace Deflate
{
    class Huffman_Tree
    {
        class Node
        {
        public:
            int key;
            Node* left;
            Node* right;

            Node(const int key, Node* left, Node* right) : key(key), left(left), right(right)
            {}

            ~Node()
            {
                delete left;
                delete right;
            }

            friend std::ostream& operator<<(std::ostream& os, const Node* node)
            {
                if (node == nullptr)
                    return os << "null";
                os << node->key << " l:" << node->left << " r:" << node->right;

                return os;
            }
        };

        static void add(Node* node, int key, int path, int path_length);

        [[nodiscard]] std::map<int, std::vector<int>> symbols_per_code_length() const;

        Node* root_;

        struct ascending
        {
            bool operator()(const std::pair<int, Node*>& a, const std::pair<int, Node*>& b) const
            {
                return a.first > b.first;
            }
        };

    public:
        /** \brief Computes canonical codes in a symbol => (code, code_length) dict */
        [[nodiscard]] std::map<int, std::pair<int, int>> canonical_codes(const int max_bit_length) const;

        /** \brief Builds a Huffman tree with a symbol => num_occurrences dict */
        explicit Huffman_Tree(const std::unordered_map<int, int>& frequency_table);

        /** \brief Builds a Huffman tree with a symbol => (code, code_length) dict */
        explicit Huffman_Tree(const std::map<int, std::pair<int, int>>& tree);

        ~Huffman_Tree()
        { delete root_; }

        /** \brief Reads a key using the tree codes
         * @param reader The deflate reader used to read the dat
         */
        int read_key(Stream_Reader& reader) const;

        friend std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree);
    };
}
