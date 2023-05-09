#pragma once
#include <map>
#include <ostream>

class Huffman_Tree
{
    static char* sort_by_frequency(const std::map<char, int>& frequency_table);
    
    class Node
    {
    public:
        char val;
        Node* left;
        Node* right;
        Node(const char val, Node* left, Node* right) : val(val), left(left), right(right) {}
        ~Node(){delete left; delete right;}

        friend std::ostream& operator<<(std::ostream& os, const Node* node)
        {
            if(node == nullptr)
                return os << 'null';
            os << node->val << " l:" << node->left << " r:" << node->right;

            return os;
        }
    };
    Node* root_;
public:
    explicit Huffman_Tree(const std::map<char, int>& frequency_table);
    ~Huffman_Tree(){delete root_;}
    friend std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree);
};
