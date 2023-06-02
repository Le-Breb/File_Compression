#pragma once
#include <map>
#include <ostream>
#include <vector>
#include <unordered_map>
#include "Main.h"

namespace Deflate {
    class Huffman_Tree
    {
        class Node
        {
        public:
            int key;
            Node* left;
            Node* right;
            Node(const int key, Node* left, Node* right) : key(key), left(left), right(right) {}
            ~Node(){delete left; delete right;}

            friend std::ostream& operator<<(std::ostream& os, const Node* node)
            {
                if(node == nullptr)
                    return os << "null";
                os << node->key << " l:" << node->left << " r:" << node->right;

                return os;
            }
        };

        void generate_chars_of_bit_length(const Node& node, int bit_length);
        static void add(Node* node, int key, int path, int path_length);

        Node* root_;
    public:
        std::map<int, int> chars_of_bit_length_;
        explicit Huffman_Tree(const std::unordered_map<int, int> &frequency_table);
        explicit Huffman_Tree(const std::map<int, std::pair<int, int>>& tree);
        ~Huffman_Tree(){delete root_;}
        int read_key(Stream_Reader& reader) const;
        friend std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree);
    };
}
