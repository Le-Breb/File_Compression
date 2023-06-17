//
// Created by matmu on 16/06/2023.
//

#include "Memory.h"

namespace Deflate
{
    void Memory::Clean()
    {
        for (auto& h : head)
            h = -1;
        for (auto& p : prev)
            p = -1;
        for (auto& l : lit_len_frequency_table)
            l = 0;
        for (auto& d : dist_frequency_table)
            d = 0;
        for (auto& c : code_lengths_frequency_table)
            c = 0;

        //for (int i = 0; i < num_symbols; ++i)
        //    delete symbols[i];
        num_symbols = 0;
    }

    Memory::Memory()
    {
        Clean();
    }
} // Deflate