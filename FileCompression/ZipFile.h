#pragma once
#include <cstdint>
#include <fstream>
#include <map>
#include <vector>

#include "MS-DOS/Date.h"
#include "MS-DOS/Time.h"


class File;
class EOCD;

class ZipFile
{
private:
    EOCD* find_eocd(std::ifstream& in);
    void register_files(std::ifstream& in, const int& offset_of_start_of_central_directory, const int& central_directory_size);
    void list_files() const;
    MS_DOS::Date* creation_date_;
    MS_DOS::Time* creation_time_;
    static MS_DOS::Date get_date_from_system();
    static MS_DOS::Time get_time_from_system();
public:
    static constexpr unsigned int current_version = 20;
    static constexpr unsigned int disk_number = 0;

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
            v1_0 = 10,
            v1_1 = 11,
            v2_0 = 20,
            v2_1 = 21,
            v2_5 = 25,
            v2_6 = 26,
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

        friend std::ostream& operator<<(std::ostream& os, const version_made_by& version_made_by)
        {
            switch (version_made_by) { case version_made_by::MS_DOS: break;
            case version_made_by::Amiga: os << "Amiga"; break;
            case version_made_by::OpenVMS: os << "OpenVMS"; break;
            case version_made_by::UNIX: os << "UNIX"; break;
            case version_made_by::VM_CMS: os << "VM_CMS"; break;
            case version_made_by::Atari_ST: os << "Atari_ST"; break;
            case version_made_by::OS_2: os << "OS_2"; break;
            case version_made_by::Macintosh: os << "Macintosh"; break;
            case version_made_by::Z_System: os << "Z_System"; break;
            case version_made_by::CP_M: os << "CP_M"; break;
            case version_made_by::Windows_NTFS: os << "Windows_NTFS"; break;
            case version_made_by::MVS: os << "MVS"; break;
            case version_made_by::VSE: os << "VSE"; break;
            case version_made_by::Acorn_RISC: os << "Acorn_RISC"; break;
            case version_made_by::VFAT: os << "VFAT"; break;
            case version_made_by::Alternate_MVS: os << "Alternate_MVS"; break;
            case version_made_by::BeOS: os << "BeOS"; break;
            case version_made_by::Tandem: os << "Tandem"; break;
            case version_made_by::OS_400: os << "OS_400"; break;
            case version_made_by::OS_X: os << "OS_X"; break;
            default: os << "Unknown"; break;
            }

            return os;
        }

        friend std::ostream& operator<<(std::ostream& os, const version_needed_to_extract& version_needed_to_extract)
        {
            switch (version_needed_to_extract) { case version_needed_to_extract::v1_0: break;
            case version_needed_to_extract::v1_1: os << "v1_1"; break;
            case version_needed_to_extract::v2_0: os << "v2_0"; break;
            case version_needed_to_extract::v2_1: os << "v2_1"; break;
            case version_needed_to_extract::v2_5: os << "v2_5"; break;
            case version_needed_to_extract::v2_6: os << "v2_6"; break;
            case version_needed_to_extract::v4_5: os << "v4_5"; break;
            case version_needed_to_extract::v4_6: os << "v4_6"; break;
            case version_needed_to_extract::v5_0: os << "v5_0"; break;
            case version_needed_to_extract::v6_1: os << "v6_1"; break;
            case version_needed_to_extract::v6_2: os << "v6_2"; break;
            case version_needed_to_extract::v6_3: os << "v6_3"; break;
            case version_needed_to_extract::v6_4: os << "v6_4"; break;
            case version_needed_to_extract::v6_5: os << "v6_5"; break;
            case version_needed_to_extract::v6_6: os << "v6_6"; break;
            case version_needed_to_extract::v6_7: os << "v6_7"; break;
            case version_needed_to_extract::v6_8: os << "v6_8"; break;
            case version_needed_to_extract::v6_9: os << "v6_9"; break;
            case version_needed_to_extract::v6_10: os << "v6_10"; break;
            case version_needed_to_extract::v6_11: os << "v6_11"; break;
            case version_needed_to_extract::v6_12: os << "v6_12"; break;
            case version_needed_to_extract::v6_13: os << "v6_13"; break;
            case version_needed_to_extract::v6_14: os << "v6_14"; break;
            case version_needed_to_extract::v6_15: os << "v6_15"; break;
            case version_needed_to_extract::v6_16: os << "v6_16"; break;
            case version_needed_to_extract::v6_17: os << "v6_17"; break;
            case version_needed_to_extract::v6_18: os << "v6_18"; break;
            case version_needed_to_extract::v6_19: os << "v6_19"; break;
            case version_needed_to_extract::v6_20: os << "v6_20"; break;
            case version_needed_to_extract::v6_21: os << "v6_21"; break;
            case version_needed_to_extract::v6_22: os << "v6_22"; break;
            case version_needed_to_extract::v6_23: os << "v6_23"; break;
            case version_needed_to_extract::v6_24: os << "v6_24"; break;
            case version_needed_to_extract::v6_25: os << "v6_25"; break;
            case version_needed_to_extract::v6_26: os << "v6_26"; break;
            case version_needed_to_extract::v6_27: os << "v6_27"; break;
            case version_needed_to_extract::v6_28: os << "v6_28"; break;
            case version_needed_to_extract::v6_29: os << "v6_29"; break;
            case version_needed_to_extract::v6_30: os << "v6_30"; break;
            case version_needed_to_extract::v6_31: os << "v6_31"; break;
            case version_needed_to_extract::v6_32: os << "v6_32"; break;
            case version_needed_to_extract::v6_33: os << "v6_33"; break;
            case version_needed_to_extract::v6_34: os << "v6_34"; break;
            case version_needed_to_extract::v6_35: os << "v6_35"; break;
            case version_needed_to_extract::v6_36: os << "v6_36"; break;
            case version_needed_to_extract::v6_37: os << "v6_37"; break;
            case version_needed_to_extract::v6_38: os << "v6_38"; break;
            case version_needed_to_extract::v6_39: os << "v6_39"; break;
            case version_needed_to_extract::v6_40: os << "v6_40"; break;
            case version_needed_to_extract::v6_41: os << "v6_41"; break;
            case version_needed_to_extract::v6_42: os << "v6_42"; break;
            case version_needed_to_extract::v6_43: os << "v6_43"; break;
            case version_needed_to_extract::v6_44: os << "v6_44"; break;
            case version_needed_to_extract::v6_45: os << "v6_45"; break;
            case version_needed_to_extract::v6_46: os << "v6_46"; break;
            default: os << "unknown"; break;
            };

            return os;
        }

        friend std::ostream& operator<<(std::ostream& os, const general_purpose_bit_flag& obj)
        {
            if (static_cast<int>(obj) == 0)
            {
                os << "None";
                return os;
            }
            for (auto& flag : general_purpose_bit_flag_to_string)
            {
                if (static_cast<int>(obj) & static_cast<int>(flag.first))
                {
                    os << flag.second << " ";
                }
            }

            return os;
        }

        friend std::ostream& operator<<(std::ostream& os, const compression_method& obj)
        {
            switch (obj)
            {
                case compression_method::Stored: os << "stored"; break;
                case compression_method::Shrunk: os << "shrunk"; break;
                case compression_method::Reduced_1: os << "reduced_with_compression_factor_1"; break;
                case compression_method::Reduced_2: os << "reduced_with_compression_factor_2"; break;
                case compression_method::Reduced_3: os << "reduced_with_compression_factor_3"; break;
                case compression_method::Reduced_4: os << "reduced_with_compression_factor_4"; break;
                case compression_method::Imploded: os << "imploded"; break;
                case compression_method::Reserved_1: os << "reserved_1"; break;
                case compression_method::Deflated: os << "deflated"; break;
                case compression_method::Enhanced_Deflated: os << "enhanced_deflated"; break;
                case compression_method::PKWare_DCL_Implode: os << "pkware_dcl_imploded"; break;
                case compression_method::BZIP2: os << "bzip2"; break;
                case compression_method::Reserved_2: os << "reserved_2"; break;
                case compression_method::LZMA: os << "lzma"; break;
                case compression_method::Reserved_3: os << "reserved_3"; break;
                case compression_method::Reserved_4: os << "reserved_4"; break;
                case compression_method::IBM_TERSE: os << "ibm_terse"; break;
                case compression_method::IBM_LZ77_z: os << "ibm_lz77_z"; break;
                case compression_method::PPMD: os << "ppmd_version_i_rev_1"; break;
                default: os << "unknown"; break;
                };

            return os;
        }
        
        const static std::map<general_purpose_bit_flag, std::string> general_purpose_bit_flag_to_string;
    };

    ~ZipFile();

    static void write_empty_zip_file(const char* filename);

    void add_file(const char *path_on_disk, const char *path_in_zip);

    void write(const char* filename) const;

    ZipFile();

    explicit ZipFile(const char* filename);
};
