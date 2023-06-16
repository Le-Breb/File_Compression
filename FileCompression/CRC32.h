#pragma once

typedef unsigned char Byte;

class CRC32
{
private:
    unsigned lookup_table_[256];
    const unsigned reversed_polynomial_ = 0xEDB88320;
public:
    CRC32();

    unsigned compute(const char* bytes, int length) const;
};

// Doc http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
// http://www.sunshine2k.de/coding/javascript/crc/crc_js.html0x2477CDF073 73