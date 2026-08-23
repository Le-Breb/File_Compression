# File Compression

A from-scratch C++20 implementation of the **DEFLATE** algorithm ([RFC 1951](https://www.ietf.org/rfc/rfc1951.txt)) and the **ZIP** file format — no `zlib`, `miniz`, or any other compression library involved. Every layer, from LZ77 match finding to the ZIP container's local/central file headers, is hand-written.

Correctness was validated by round-tripping files through Windows' built-in ZIP support both ways: compressing with this program and extracting with Windows Explorer, and compressing with Explorer and extracting with this program. Once that was solid, the match finder was rebuilt around the hash-chain approach used by `zlib`, after reading through its source for ideas.

## Features

- **LZ77** over a 32 KB sliding window: every byte is emitted either as a literal or as a `(length, distance)` back-reference into the last 32 KB of already-processed data.
- **Hash-chain match finding**, adapted from `zlib`: 3-byte sequences are hashed into buckets holding the most recent position, with a parallel `prev` array threading earlier positions of the same hash into a chain that's walked backwards to find the longest match, capped at 1024 probes so pathological inputs stay fast. A one-byte lookahead (lazy matching) checks whether waiting one more position yields a better match before committing to the current one.
- **Two canonical Huffman trees per block** — one for literals/lengths and the end-of-block marker, one for distances — built from each block's actual symbol frequencies, with the tree shapes themselves compressed using the three RFC 1951 repeat codes.
- **Per-block mode selection**: each block is compressed with stored, fixed-Huffman, and dynamic-Huffman coding, and whichever produces the smallest output is written.
- **Full ZIP container**: local file headers (LFH), central directory file headers (CDFH), the end-of-central-directory record (EOCD), CRC32 checksums, and MS-DOS-encoded timestamps, assembled around the DEFLATE stream so the result opens in any standard ZIP tool.
- Reading (`ZipFile(filename)`) and writing (`ZipFile::write`) of ZIP archives, including whole-folder archiving via `add_folder`.

## Project layout

```
FileCompression/
├── Compressors/Deflate/   DEFLATE codec: match finder, Huffman trees, bit-level reader/writer
├── MS-DOS/                MS-DOS date/time encoding used by ZIP headers
├── ZipFile.*, File.*      ZIP archive model (list of files) and per-file entry logic
├── LFH.*, CDFH.*, EOCD.*  Local file header / central directory file header / end-of-central-directory records
├── CRC32.*                CRC32 checksum implementation
├── Base64.*               Base64 encode/decode helper
├── Exceptions.*           Custom exception types
└── FileCompression.cpp    Entry point / manual test driver
```

## Building

Requires a C++20 compiler and CMake 3.25+.

```bash
cmake -S FileCompression -B FileCompression/cmake-build-debug
cmake --build FileCompression/cmake-build-debug
```

A Visual Studio solution (`FileCompression.sln`) is also provided.

> **Note:** `ZipFile::add_file` currently uses the Win32 API (`GetFileAttributesExA`, `FileTimeToSystemTime`) to read file timestamps and attributes, so building `add_file`/`add_folder` requires Windows. The DEFLATE codec itself (`Compressors/Deflate`) is platform-independent.

## Usage

```cpp
#include "ZipFile.h"

ZipFile zip;
zip.add_folder("path/to/folder", "folder");   // add a whole directory, recursively
zip.write("archive.zip");                     // write it out as a real .zip file

ZipFile existing("archive.zip");               // read an existing archive
existing.list_files();                         // print its contents
```

The DEFLATE codec can also be used directly:

```cpp
#include "Compressors/Deflate/Main.h"

std::vector<Byte> compressed = Deflate::Main::deflate(data, size);
std::vector<Byte> original   = Deflate::Main::inflate(compressed);
```

`Deflate::Main::test()` compresses and round-trips every file in `../Data/calgaryCorpus` (the [Calgary Corpus](https://corpus.canterbury.ac.nz/descriptions/#calgary)) and prints size, ratio, and timing per file — useful as a quick benchmark/regression check when the corpus is present locally.

## References used

- [ZIP file format (Wikipedia)](https://en.wikipedia.org/wiki/ZIP_(file_format))
- [DEFLATE (Wikipedia)](https://en.wikipedia.org/wiki/Deflate)
- [PKWARE .ZIP APPNOTE.TXT](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT)
- [RFC 1951 — DEFLATE Compressed Data Format Specification](https://www.ietf.org/rfc/rfc1951.txt)
- [zlib source (`deflate.c`)](https://github.com/madler/zlib/blob/master/deflate.c), for the hash-chain match-finding strategy

## Status

Personal project, built in 2023. Not currently packaged as a reusable library — see `FileCompression.cpp` for example driver code.
