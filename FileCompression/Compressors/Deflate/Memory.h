//
// Created by matmu on 16/06/2023.
//

#ifndef FILECOMPRESSION_MEMORY_H
#define FILECOMPRESSION_MEMORY_H

#include "Main.h"

/** \brief Memory used by the compressor */
namespace Deflate
{

    class Memory
    {
    public:
        Memory();

        int head[Main::hash_size];
        int prev[Main::window_size];
        int lit_len_frequency_table[286];
        int dist_frequency_table[30];
        int code_lengths_frequency_table[19];

        Match symbols[Main::MAX_SYMBOLS_PER_BLOCK];
        int num_symbols = 0;

        /** \brief Resets the memory to its initial state */
        void Clean();
    };

} // Deflate

#endif //FILECOMPRESSION_MEMORY_H
