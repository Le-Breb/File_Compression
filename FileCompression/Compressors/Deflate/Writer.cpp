//
// Created by matmu on 04/06/2023.
//

#include <stdexcept>
#include "Writer.h"


void Deflate::Writer::write_number(int number, int bit_length)
{
    for (int i = 0; i < bit_length; ++i)
        write_bit(number & (1 << i));
}

void Deflate::Writer::write_bit(bool bit)
{
    curr_byte |= bit << bit_index_++;
    if (bit_index_ == 8)
        write_curr_byte();
}

void Deflate::Writer::write_curr_byte()
{
    bit_index_ = 0;
    data->emplace_back(curr_byte);
    curr_byte = 0;
}

void Deflate::Writer::close()
{
    write_curr_byte();
}

void Deflate::Writer::write_code(int code, int bit_length)
{
    for (int i = bit_length - 1; i >= 0; --i)
        write_bit(code & (1 << i));
}