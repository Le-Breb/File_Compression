#include "Huffman_Tree.h"
#include <iostream>
#include <queue>


void Deflate::Huffman_Tree::generate_chars_of_bit_length(const Node& node, const int bit_length)
{
    if (node.key == -1)
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

Deflate::Huffman_Tree::Huffman_Tree(const std::unordered_map<int, int> &frequency_table)
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

void Deflate::Huffman_Tree::add(Node* node, const int key, const int path, const int path_length)
{
    if(!path_length)
    {
        node->key = key;
        return;
    }
    if(path & (1 << (path_length - 1)))
    {
        if(node->right == nullptr)
            node->right = new Node(-1, nullptr, nullptr);
        add(node->right, key, path, path_length - 1);        
    }
    else
    {
        if(node->left == nullptr)
            node->left = new Node(-1, nullptr, nullptr);
        add(node->left, key, path, path_length - 1);
    }
}

Deflate::Huffman_Tree::Huffman_Tree(const std::map<int, std::pair<int, int>>& tree)
{
    root_ = new Node(-1, nullptr, nullptr);

    for (const auto p : tree)
        add(root_, p.first, p.second.first, p.second.second);

    generate_chars_of_bit_length(*root_, 0);
}

int Deflate::Huffman_Tree::read_key(Stream_Reader& reader) const
{
    const Node* current = root_;
    while(current->key == -1)
    {
        if (reader.read_bit())
            current = current->right;
        else 
            current = current->left;
    }

    return current->key;
}

std::ostream &Deflate::operator<<(std::ostream &os, const Deflate::Huffman_Tree &huffman_tree) {
    os << huffman_tree.root_;

    return os;
}

std::map<int, int> Deflate::Huffman_Tree::code_lengths() const {
    std::map<int, int> dict;

    Node* n = root_;
    std::queue<std::pair<Node*, int>> q;
    q.emplace(n, 0);
    while(!q.empty())
    {
        auto p = q.front();
        q.pop();
        if(p.first->key != -1)
            dict[p.first->key] = p.second;
        else
        {
            if (p.first->left != nullptr)
                q.emplace(p.first->left, p.second + 1);
            if (p.first->right != nullptr)
                q.emplace(p.first->right, p.second + 1);
        }
    }

    return dict;
}

