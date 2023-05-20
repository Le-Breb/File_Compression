#include "Huffman_Tree.h"

#include <algorithm>
#include <iostream>


void Huffman_Tree::generate_chars_of_bit_length(const Node& node, const int bit_length)
{
    if (node.key == 0)
    {
        if (node.left != nullptr)
            generate_chars_of_bit_length(*node.left, bit_length + 1);
        if (node.right != nullptr)
            generate_chars_of_bit_length(*node.right, bit_length + 1);
    }
    else
    {
        if (!chars_of_bit_length_.contains(bit_length))
            chars_of_bit_length_[bit_length] = 1;
        else
            chars_of_bit_length_[bit_length]++;
    }
}

void Huffman_Tree::add(Node* node, const char key, const int path, const int path_length)
{
    if(!path_length)
    {
        node->key = key;
        return;
    }
    if(path & (1 << (path_length - 1)))
    {
        if(node->right == nullptr)
        node->right = new Node(0, nullptr, nullptr);
        add(node->right, key, path, path_length - 1);        
    }
    else
    {
        if(node->left == nullptr)
            node->left = new Node(0, nullptr, nullptr);
        add(node->left, key, path, path_length - 1);
    }
}

Huffman_Tree::Huffman_Tree(const std::map<char, int>& frequency_table)
{   
    std::map<int, Node*> l;
    for(const auto& el : frequency_table)
        l[el.second] = new Node(el.first, nullptr, nullptr);

    while(l.size() > 1)
    {
        Node* n1 = l.begin()->second;
        Node* n2 = (++l.begin())->second;
        int freq = l.begin()->first + (++l.begin())->first;
        Node* n = new Node(0, n2, n1);
        l.erase(l.begin()->first);
        l.erase(l.begin()->first);
        l[freq] = n;
    }

    root_ = l.begin()->second;
    generate_chars_of_bit_length(*root_, 0);
}

Huffman_Tree::Huffman_Tree(const std::map<char, std::pair<int, int>>& tree)
{
    root_ = new Node(0, nullptr, nullptr);

    for (auto p : tree)
        add(root_, std::get<0>(p), std::get<0>(std::get<1>(p)), std::get<1>(std::get<1>(p)));

    generate_chars_of_bit_length(*root_, 0);
}

int Huffman_Tree::read_key(Deflate::Stream_Reader& reader) const
{
    const Node* current = root_;
    while(current->key == 0)
        current = reader.read_bit() ? current->right : current->left;

    return current->key;
}

std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree)
{
    os << huffman_tree.root_;
    
    return os;
}
