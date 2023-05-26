cmake_minimum_required(VERSION 3.25)
project(FileCompression)

set(CMAKE_CXX_STANDARD 20)

add_executable(FileCompression
    Compressors/Huffman_Tree/Huffman_Tree.cpp
    Compressors/Huffman_Tree/Huffman_Tree.h
    Compressors/Deflate.cpp
    Compressors/Deflate.h
    MS-DOS/Date.cpp
    MS-DOS/Date.h
    MS-DOS/Time.cpp
    MS-DOS/Time.h
    CDFH.cpp
    CDFH.h
    CRC32.cpp
    CRC32.h
    EOCD.cpp
    EOCD.h
    Exceptions.cpp
    Exceptions.h
    File.cpp
    File.h
    FileCompression.cpp
    LFH.cpp
    LFH.h
    ZipFile.cpp
    ZipFile.h Base64.cpp Base64.h)
