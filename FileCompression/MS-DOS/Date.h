#pragma once

#include <ostream>
#include <Windows.h>

typedef unsigned char Byte;
namespace MS_DOS
{
    class Date
    {
    public:
        const Byte bytes_[2];

        explicit Date(const Byte bytes[2]) : bytes_{bytes[0], bytes[1]}
        {}

        explicit Date(const SYSTEMTIME& t);

        Date(unsigned short year, unsigned short month, unsigned short day);

        [[nodiscard]] unsigned short get_year() const;

        [[nodiscard]] unsigned short get_month() const;

        [[nodiscard]] unsigned short get_day() const;

        Date operator=(const Date& date) const;

        friend std::ostream& operator<<(std::ostream& os, const Date& date);
    };
}
