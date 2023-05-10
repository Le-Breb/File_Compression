#include "Huffman_Tree.h"

#include <algorithm>
#include <iostream>


void Huffman_Tree::generate_chars_of_bit_length(const Node& node, const int bit_length)
{
    if (node.val == 0)
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

std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree)
{
    os << huffman_tree.root_;
    
    return os;
}
