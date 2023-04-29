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
            ". The maximum supported version is: " + std::to_string(ZipFile::max_supported_version) + ".").c_str());
    }
};
