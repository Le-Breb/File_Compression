//
// Created by matmu on 04/06/2023.
//

#include "Stream_Writer.h"


Deflate::Stream_Writer::Stream_Writer(const char *path, const unsigned int offset)  : out_(path, std::ios::binary){
    out_.seekp(offset);
}

void Deflate::Stream_Writer::write_number(int number, int bit_length) {
    for(int i = bit_length - 1; i >= 0; --i)
        write_bit(number & (1 << i));
}

void Deflate::Stream_Writer::write_bit(bool bit) {
    curr_byte |= bit << bit_index_++;
    if (bit_index_ == 8)
        write_curr_byte();
}

void Deflate::Stream_Writer::write_curr_byte() {
    bit_index_ = 0;
    out_.write(&curr_byte, 1);
    curr_byte = 0;
}

void Deflate::Stream_Writer::close() {
    write_curr_byte();
    out_.close();
}

void Deflate::Stream_Writer::write_bytes(const std::vector<bool> &bits) {
    throw std::runtime_error("Not implemented !");
}

void Deflate::Stream_Writer::write_bytes(const std::vector<unsigned char> &bytes) {
    throw std::runtime_error("Not implemented !");
}

void Deflate::Stream_Writer::write_code(int code, int bit_length) {
    for (int i = 0; i < bit_length; ++i)
        write_bit(code & (1 << i));
}
