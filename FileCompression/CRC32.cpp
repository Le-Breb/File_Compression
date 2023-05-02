#include "CRC32.h"

#include <iostream>

CRC32::CRC32()
{
    // Initialize lookup table
    for(int i = 0; i < 256; i ++)
    {
        unsigned crc = i;
        for(int j = 0; j < 8; j ++)
        {
            crc = (crc & 1) ? (crc >> 1) ^ reversed_polynomial_ : (crc >> 1);
        }
        lookup_table_[i] = crc;
    }
}

unsigned CRC32::compute(const char* bytes, int length) const
{
    unsigned crc = 0xffffffff;
    while(length--)
        crc = (crc >> 8) ^ lookup_table_[(crc & 0xff) ^ *bytes++];

    return ~crc;
}
