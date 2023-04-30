#pragma once
#include <exception>
#include <string>

#include "ZipFile.h"

class unsupported_zip_version final : public std::exception
{
private:
    unsigned unsupported_version_;

public:
    explicit unsupported_zip_version(const unsigned unsupported_version) : unsupported_version_(unsupported_version_) {}
    char * what () {
        return const_cast<char*>(("Unsupported zip version. You used version: " + std::to_string(unsupported_version_) +
            ". The maximum supported version is: " + std::to_string(ZipFile::current_version) + ".").c_str());
    }
};
class invalid_file final : public std::exception
{

public:
    enum class Reason
    {
        NO_EOCD
    };
    explicit invalid_file(const Reason reason) : reason_(reason) {}
    char * what () {
        return const_cast<char*>(("Invalid file. Reason: " + get_reason()).c_str());
    }

    std::string get_reason() const
    {
        switch (reason_)
        {
        case Reason::NO_EOCD:
            return "The file does not contain an EOCD record.";
        default:
            return "Unknown reason.";
        }
    }
private:
    Reason reason_;
};
