//
// Created by matmu on 04/06/2023.
//

#include "StreamWriter.h"


Deflate::StreamWriter::StreamWriter(const char* path, const unsigned int offset) : out_(path, std::ios::binary)
{
    out_.seekp(offset);
}

void Deflate::StreamWriter::writeNumber(int number, int bit_length)
{
    for (int i = bit_length - 1; i >= 0; --i)
        writeBit(number & (1 << i));
}

void Deflate::StreamWriter::writeBit(bool bit)
{
    curr_byte |= bit << bit_index_++;
    if (bit_index_ == 8)
        writeCurrByte();
}

void Deflate::StreamWriter::writeCurrByte()
{
    bit_index_ = 0;
    out_.write(&curr_byte, 1);
    curr_byte = 0;
}

void Deflate::StreamWriter::close()
{
    writeCurrByte();
    out_.close();
}

void Deflate::StreamWriter::writeBytes(const std::vector<bool>& bits)
{
    throw std::runtime_error("Not implemented !");
}

void Deflate::StreamWriter::writeBytes(const std::vector<Byte>& bytes)
{
    throw std::runtime_error("Not implemented !");
}

void Deflate::StreamWriter::writeCode(int code, int bit_length)
{
    for (int i = 0; i < bit_length; ++i)
        writeBit(code & (1 << i));
}
