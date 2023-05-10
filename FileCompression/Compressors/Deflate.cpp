#include "Deflate.h"

std::tuple<char*, int> Deflate::compress(const char* data, int size)
{
    char* res = new char[size + 1];
    res[0] = data[0] >> 3;
    for (int i = 1; i < size; i++)
        res[i] = (data[i - 1] & 7) << 5 | data[i] >> 3;
    res[size] = (data[size - 1] & 7) >> 5;  

    return {res, size + 1};
}
