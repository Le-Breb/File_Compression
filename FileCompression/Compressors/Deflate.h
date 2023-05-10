#pragma once
#include <tuple>

class Deflate
{
public:
    static std::tuple<char*, int> compress(const char* data, int size);
};
