#pragma once
#include <cstdint>
#include <fstream>
#include <vector>


class File;
class EOCD;

class ZipFile
{
private:
    EOCD* find_eocd(std::ifstream& in);
public:
    static constexpr int current_version = 20;
    static constexpr int disk_number = 0;

    std::vector<File*> files;
    
    struct Fields
    {
        enum class version_made_by : uint16_t
        {
            MS_DOS = 0,
            Amiga = 1,
            OpenVMS = 2,
            UNIX = 3,
            VM_CMS = 4,
            Atari_ST = 5,
            OS_2 = 6,
            Macintosh = 7,
            Z_System = 8,
            CP_M = 9,
            Windows_NTFS = 10,
            MVS = 11,
            VSE = 12,
            Acorn_RISC = 13,
            VFAT = 14,
            Alternate_MVS = 15,
            BeOS = 16,
            Tandem = 17,
            OS_400 = 18,
            OS_X = 19,
            Unknown = 20
        };

        enum class version_needed_to_extract : uint16_t
        {
            v1_0 = 0,
            v1_1 = 1,
            v2_0 = 2,
            v2_1 = 3,
            v2_5 = 4,
            v2_6 = 5,
            v4_5 = 45,
            v4_6 = 46,
            v5_0 = 50,
            v6_1 = 61,
            v6_2 = 62,
            v6_3 = 63,
            v6_4 = 64,
            v6_5 = 65,
            v6_6 = 66,
            v6_7 = 67,
            v6_8 = 68,
            v6_9 = 69,
            v6_10 = 70,
            v6_11 = 71,
            v6_12 = 72,
            v6_13 = 73,
            v6_14 = 74,
            v6_15 = 75,
            v6_16 = 76,
            v6_17 = 77,
            v6_18 = 78,
            v6_19 = 79,
            v6_20 = 80,
            v6_21 = 81,
            v6_22 = 82,
            v6_23 = 83,
            v6_24 = 84,
            v6_25 = 85,
            v6_26 = 86,
            v6_27 = 87,
            v6_28 = 88,
            v6_29 = 89,
            v6_30 = 90,
            v6_31 = 91,
            v6_32 = 92,
            v6_33 = 93,
            v6_34 = 94,
            v6_35 = 95,
            v6_36 = 96,
            v6_37 = 97,
            v6_38 = 98,
            v6_39 = 99,
            v6_40 = 100,
            v6_41 = 101,
            v6_42 = 102,
            v6_43 = 103,
            v6_44 = 104,
            v6_45 = 105,
            v6_46 = 106,
    };

        enum class compression_method : uint16_t
        {
            Stored = 0,
            Shrunk = 1,
            Reduced_1 = 2,
            Reduced_2 = 3,
            Reduced_3 = 4,
            Reduced_4 = 5,
            Imploded = 6,
            Reserved_1 = 7,
            Deflated = 8,
            Enhanced_Deflated = 9,
            PKWare_DCL_Implode = 10,
            Reserved_2 = 11,
            BZIP2 = 12,
            Reserved_3 = 13,
            LZMA = 14,
            Reserved_4 = 15,
            Reserved_5 = 16,
            Reserved_6 = 17,
            IBM_TERSE = 18,
            IBM_LZ77_z = 19,
            MP3 = 94,
            XZ = 95,
            JPEG = 96,
            WavPack = 97,
            PPMD = 98,
            AE_x = 99,
            Unknown = 100
        };

        enum class general_purpose_bit_flag : uint16_t
        {
            Encrypted = 0x0001,
            Compression_option_1 = 0x0002,
            Compression_option_2 = 0x0004,
            Data_descriptor = 0x0008,
            Enhanced_deflation = 0x0010,
            Compressed_patched_data = 0x0020,
            Strong_encryption = 0x0040,
            UTF_8 = 0x0800,
            Mask_header_values = 0x2000,
            Reserved = 0x4000,
            Reserved_2 = 0x8000,
            None = 0x0000
        };
    };

    ~ZipFile();

    static void write_empty_zip_file(const char* filename);

    void add_file(const char* filename);

    void write(const char* filename);

    ZipFile();

    explicit ZipFile(const char* filename);
};
