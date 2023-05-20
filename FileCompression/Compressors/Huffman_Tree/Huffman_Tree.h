#pragma once
#include <map>
#include <ostream>
#include <vector>

#include "../Deflate.h"

class Huffman_Tree
{
    class Node
    {
    public:
        char key;
        Node* left;
        Node* right;
        Node(const char val, Node* left, Node* right) : key(val), left(left), right(right) {}
        ~Node(){delete left; delete right;}

        friend std::ostream& operator<<(std::ostream& os, const Node* node)
        {
            if(node == nullptr)
                return os << "null";
            os << node->key << " l:" << node->left << " r:" << node->right;

            return os;
        }
    };
    
    std::map<int, int> chars_of_bit_length_;
    void generate_chars_of_bit_length(const Node& node, int bit_length);
    static void add(Node* node, char key, int path, int path_length);
    
    Node* root_;
public:
    explicit Huffman_Tree(const std::map<char, int>& frequency_table);
    explicit Huffman_Tree(const std::map<char, std::pair<int, int>>& tree);
    ~Huffman_Tree(){delete root_;}
    int read_key(Deflate::Stream_Reader& reader) const;
    friend std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree);
};
