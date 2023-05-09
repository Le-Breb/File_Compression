#include "Huffman_Tree.h"

#include <algorithm>
#include <iostream>
#include <vector>

bool cmp(const std::pair<char, int>& a, const std::pair<char, int>& b)
{
    return a.second < b.second;
}

char* Huffman_Tree::sort_by_frequency(const std::map<char, int>& frequency_table)
{
    char* sorted_chars = new char[frequency_table.size()];
    std::vector<std::pair<char, int>> tmp;
    tmp.reserve(frequency_table.size());
    for(const auto& p : frequency_table)
        tmp.emplace_back(p);

    std::sort(std::begin(tmp), std::end(tmp), cmp);

    int i = 0;
    for(const auto& el : tmp)
    {
        sorted_chars[i] = el.first;
        i++;
    }
    
    return sorted_chars;
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
}

std::ostream& operator<<(std::ostream& os, const Huffman_Tree& huffman_tree)
{
    os << huffman_tree.root_;
    
    return os;
}
